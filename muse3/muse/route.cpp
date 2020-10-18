//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.cpp,v 1.18.2.3 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011, 2015 Tim E. Real (terminator356 on sourceforge)
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

#include <QWidget>
#include <QPixmap>

#include "song.h"
#include "route.h"
#include "audio.h"
#include "track.h"
#include "synth.h"
#include "audiodev.h"
#include "xml.h"
#include "mididev.h"
#include "midiport.h"
#include "icons.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "strntcpy.h"
#include "globals.h"

//#define ROUTE_DEBUG 

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

namespace MusECore {

const QString ROUTE_MIDIPORT_NAME_PREFIX = "MusE MidiPort ";

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

Route::Route(void* t, int ch)
      {
      jackPort = t;
      persistentJackPortName[0] = 0;
      if(MusEGlobal::checkAudioDevice())
        MusEGlobal::audioDevice->portName(jackPort, persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
      
      midiPort = -1;
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      type     = JACK_ROUTE;
      }

Route::Route(Track* t, int ch, int chans)
      {
      track    = t;
      midiPort = -1;
      channel  = ch;
      channels = chans;
      remoteChannel = -1;
      type     = TRACK_ROUTE;
      persistentJackPortName[0] = 0;
      }

Route::Route(MidiDevice* d, int ch)
{
      device   = d;  
      midiPort = -1;
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      type    = MIDI_DEVICE_ROUTE; 
      persistentJackPortName[0] = 0;
}

Route::Route(int port, int ch)  
{
      track    = 0;
      midiPort = port; 
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      type    = MIDI_PORT_ROUTE;     
      persistentJackPortName[0] = 0;
}

Route::Route(const QString& s, bool dst, int ch, int rtype)
    {
      Route node(name2route(s, dst, rtype));
      channel  = node.channel;
      if(channel == -1)
        channel = ch;
      channels = node.channels;
      remoteChannel = node.remoteChannel;
      type = node.type;
      persistentJackPortName[0] = 0;
      if(type == TRACK_ROUTE)
      {
        track = node.track;
        midiPort = -1;
      }
      else
      if(type == JACK_ROUTE)
      {  
        jackPort = node.jackPort;
        char* res = 0;
        if(jackPort && MusEGlobal::checkAudioDevice())
          res = MusEGlobal::audioDevice->portName(jackPort, persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
        if(!res)
          MusELib::strntcpy(persistentJackPortName, s.toLatin1().constData(), ROUTE_PERSISTENT_NAME_SIZE);
        midiPort = -1;
      }
      else
      if(type == MIDI_DEVICE_ROUTE)  
      {
        device = node.device;     
        midiPort = -1;
      }  
      else
      if(type == MIDI_PORT_ROUTE)    
      {
        track = 0;
        midiPort = node.midiPort;    
      }  
    }

    
Route::Route()
      {
      track    = 0;
      midiPort = -1;
      channel  = -1;
      channels = -1;
      remoteChannel = -1;
      type     = TRACK_ROUTE;
      persistentJackPortName[0] = 0;
      }

Route::Route(RouteType type_, int midi_port_num_, void* void_pointer_, int channel_, int channels_, int remote_channel_, const char* name_)
      {
      type          = type_;
      midiPort      = midi_port_num_;
      voidPointer   = void_pointer_;
      channel       = channel_;
      channels      = channels_;
      remoteChannel = remote_channel_;
      persistentJackPortName[0] = 0;
      MusELib::strntcpy(persistentJackPortName, name_, ROUTE_PERSISTENT_NAME_SIZE);
      }
      
Route::Route(const Route& a)
{
      type          = a.type;
      midiPort      = a.midiPort;
      voidPointer   = a.voidPointer;
      channel       = a.channel;
      channels      = a.channels;
      remoteChannel = a.remoteChannel;
      persistentJackPortName[0] = 0;
      strcpy(persistentJackPortName, a.persistentJackPortName);
}

Route& Route::operator=(const Route& a)
{
      type          = a.type;
      midiPort      = a.midiPort;
      voidPointer   = a.voidPointer;
      channel       = a.channel;
      channels      = a.channels;
      remoteChannel = a.remoteChannel;
      persistentJackPortName[0] = 0;
      strcpy(persistentJackPortName, a.persistentJackPortName);
      return *this;
}

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

bool addRoute(Route src, Route dst)
{
      #ifdef ROUTE_DEBUG
      fprintf(stderr, "addRoute:\n");
      #endif
      
      if (!src.isValid() || !dst.isValid())
      {
            if(!src.isValid())
              fprintf(stderr, "addRoute: invalid src\n");
            if(!dst.isValid())
              fprintf(stderr, "addRoute: invalid dst\n");
            return false;
      }
      
//      fprintf(stderr, "addRoute %d.%d:<%s> %d.%d:<%s>\n",
//         src.type, src.channel, src.name().toLatin1().constData(),
//         dst.type, dst.channel, dst.name().toLatin1().constData());
      if (src.type == Route::JACK_ROUTE) 
      {           
            if (dst.type == Route::TRACK_ROUTE) 
            {
              if (dst.track->type() != Track::AUDIO_INPUT) 
              {
                fprintf(stderr, "addRoute: source is jack, dest:%s is track but not audio input\n", dst.track->name().toLatin1().constData());
                return false;
              }
              if (dst.channel < 0) 
              {
                fprintf(stderr, "addRoute: source is jack, dest:%s is track but invalid channel:%d\n", dst.track->name().toLatin1().constData(), dst.channel);
                return false;
              }
              
              src.channel = dst.channel;
              
              if(dst.track->inRoutes()->contains(src))
              {
                fprintf(stderr, "addRoute: src track route already exists.\n");
                return false;
              }
              
              #ifdef ROUTE_DEBUG
              fprintf(stderr, "addRoute: src Jack dst track name: %s pushing source route\n", dst.track->name().toLatin1().constData());
              #endif
              
              dst.track->inRoutes()->push_back(src);
              
              return true;
            }  
            else if (dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(dst.device->deviceType() == MidiDevice::JACK_MIDI)
              {
                src.channel = dst.channel;
                
                if(dst.device->inRoutes()->contains(src))
                {
                  fprintf(stderr, "addRoute: src Jack midi route already exists.\n");
                  return false;
                }
                
                #ifdef ROUTE_DEBUG
                fprintf(stderr, "addRoute: src Jack dst Jack midi name: %s pushing source route\n", dst.device->name().toLatin1().constData());
                #endif
                
                dst.device->inRoutes()->push_back(src);
                
                return true;
              }  
              else
              {
                fprintf(stderr, "addRoute: source is Jack, but destination is not jack midi - type:%d\n", dst.device->deviceType());
                return false;
              }
            }  
            else if(dst.type == Route::JACK_ROUTE) 
            {
              // Do nothing - it's a direct Jack connection!
              return false;
            }
            else
            {
              fprintf(stderr, "addRoute: source is Jack, but destination is not track or midi - type:%d \n", dst.type);
              return false;
            }
            
            return false;
      }
      else if (dst.type == Route::JACK_ROUTE) 
      {
            if (src.type == Route::TRACK_ROUTE) 
            {
              if (src.track->type() != Track::AUDIO_OUTPUT) 
              {
                fprintf(stderr, "addRoute: destination is jack, source is track but not audio output\n");
                return false;
              }
              if (src.channel < 0) 
              {
                fprintf(stderr, "addRoute: destination is jack, source:%s is track but invalid channel:%d\n", src.track->name().toLatin1().constData(), src.channel);
                return false;
              }
              
              dst.channel = src.channel;
              
              if(src.track->outRoutes()->contains(dst))
              {
                fprintf(stderr, "addRoute: dst track route already exists.\n");
                return false;
              }
              
              #ifdef ROUTE_DEBUG
              fprintf(stderr, "addRoute: dst Jack src track name: %s pushing destination route\n", src.track->name().toLatin1().constData());
              #endif
              
              src.track->outRoutes()->push_back(dst);
              
              return true;
            }
            else if (src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(src.device->deviceType() == MidiDevice::JACK_MIDI)
              {
                dst.channel = src.channel;
                
                if(src.device->outRoutes()->contains(dst))
                {
                  fprintf(stderr, "addRoute: dst Jack midi route already exists.\n");
                  return false;
                }
                
                #ifdef ROUTE_DEBUG
                fprintf(stderr, "addRoute: dst Jack src Jack midi name: %s pushing destination route\n", src.device->name().toLatin1().constData());
                #endif
                
                if(src.device->midiPort() != -1)
                  // Initializations sysex etc. need to be sent to the new connection.
                  MusEGlobal::midiPorts[src.device->midiPort()].clearInitSent();  
                src.device->outRoutes()->push_back(dst);
                
                return true;
              }
              else  
              {
                fprintf(stderr, "addRoute: destination is Jack, but source is not jack midi - type:%d\n", src.device->deviceType());
                return false;
              }
              
              return false;
            }
            else if(src.type == Route::JACK_ROUTE) 
            {
              // Do nothing - it's a direct Jack connection!
              return false;
            }
            else
            {
              fprintf(stderr, "addRoute: destination is Jack, but source is not track or midi - type:%d \n", src.type);
              return false;
            }
            
            return false;
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  
      {
            if(dst.type != Route::TRACK_ROUTE || !dst.track->isMidiTrack())
            {
              fprintf(stderr, "addRoute: source is midi port:%d, but destination is not midi track\n", src.midiPort);
              return false;
            }
            if(dst.channel < -1 || dst.channel >= MusECore::MUSE_MIDI_CHANNELS)
            {
              fprintf(stderr, "addRoute: source is midi port:%d, but destination track channel:%d out of range\n", src.midiPort, dst.channel);
              return false;
            }       
        
            MidiPort *mp = &MusEGlobal::midiPorts[src.midiPort];
            
            // Do not allow synth ports to connect to any track. It may be useful in some cases, 
            //  may be desired later, but for now it's just a routing hassle.  p4.0.35 
            //if(mp->device() && mp->device()->isSynti())
            //  return false;
            
            bool ret1 = false;
            bool ret2 = false;
            src.channel = dst.channel;
            RouteList* rl;
            rl = mp->outRoutes();
            if(!rl->contains(dst))
            {
              rl->push_back(dst);
              ret1 = true;
            }
            rl = dst.track->inRoutes();
            if(!rl->contains(src))
            {
              rl->push_back(src);
              ret2 = true;
            }
            
            return ret1 || ret2;
      }
      else if(dst.type == Route::MIDI_PORT_ROUTE)  
      {
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            fprintf(stderr, "addRoute: destination is midi port:%d, but source is not allowed\n", dst.midiPort);
            return false;
#else        
        
            if(src.type != Route::TRACK_ROUTE || !src.track->isMidiTrack())
            {
              fprintf(stderr, "addRoute: destination is midi port:%d, but source is not midi track\n", dst.midiPort);
              return false;
            }
            if(src.channel < -1 || src.channel >= MusECore::MUSE_MIDI_CHANNELS)
            {
              fprintf(stderr, "addRoute: destination is midi port:%d, but source track channel:%d out of range\n", dst.midiPort, src.channel);
              return false;
            }       
        
            bool ret1 = false;
            bool ret2 = false;
            dst.channel = src.channel;
            RouteList* rl;
            rl = src.track->outRoutes();
            if(!rl->contains(dst))
            {
              rl->push_back(dst);
              ret1 = true;
            }
            rl = MusEGlobal::midiPorts[dst.midiPort].inRoutes();
            if(!rl->contains(src))
            {
              rl->push_back(src);
              ret2 = true;
            }
            
            return ret1 || ret2;
#endif        
      }
      else 
      {
        if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
        {
          fprintf(stderr, "addRoute: source or destination are not track routes\n");
          return false;
        }
        
        RouteList* outRoutes = src.track->outRoutes();
        if((src.channel == -1 && dst.channel != -1) || (dst.channel == -1 && src.channel != -1))
        {
          fprintf(stderr, "addRoute: source and destination are track routes but channels incompatible: src:%d dst:%d\n", src.channel, dst.channel);
          return false;
        }
        
        // Don't bother checking valid channel ranges, to support persistent routes...

        if(src.channels != dst.channels)
        {
          fprintf(stderr, "addRoute: source and destination are track routes but number of channels incompatible: src:%d dst:%d\n", src.channels, dst.channels);
          return false;
        }

        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src  TrackA, Channel  4, Remote Channel  2   dst: TrackB channel  2 Remote Channel  4
        //
        // Ex. (Handled above, not used here. For example only.) 
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        src.remoteChannel = src.channel;
        dst.remoteChannel = dst.channel;
        const int src_chan = src.channel;
        src.channel = dst.channel;
        dst.channel = src_chan;
        
        #ifdef ROUTE_DEBUG
        fprintf(stderr, "addRoute: src track ch:%d chs:%d remch:%d  dst track ch:%d chs:%d remch:%d name: %s pushing dest and source routes\n", 
          src.channel, src.channels, src.remoteChannel, dst.channel, dst.channels, dst.remoteChannel, dst.track->name().toLatin1().constData());
        #endif

        const bool o_found = outRoutes->contains(dst);
        if(o_found)
          fprintf(stderr, "addRoute: dst track route already exists in src track out routes list. Ignoring.\n");
        else
          outRoutes->push_back(dst);
        
        RouteList* inRoutes = dst.track->inRoutes();
        const bool i_found = inRoutes->contains(src);
        if(i_found)
          fprintf(stderr, "addRoute: src track route already exists in dst track out routes list. Ignoring.\n");
        else
        {
          // make sure AUDIO_AUX is processed last
          if(src.track->type() == Track::AUDIO_AUX)  // REMOVE Tim. Or not? This special aux code may not be useful or needed now. TBD
            inRoutes->push_back(src);
          else
            inRoutes->insert(inRoutes->begin(), src);
        }
        
        // Only if a route was established:
        if(!o_found || !i_found)
        {
          // Is the source an Aux Track or else does it have Aux Tracks routed to it?
          // Update the destination track's aux ref count, and all tracks it is routed to.
          if(src.track->auxRefCount())
              src.track->updateAuxRoute( src.track->auxRefCount(), dst.track );
          else 
          if(src.track->type() == Track::AUDIO_AUX)
              src.track->updateAuxRoute( 1, dst.track );
          
          return true;
        }
        
        return false;
      }
      
      return false;
}

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

bool removeRoute(Route src, Route dst)
{
  if(src.type == Route::JACK_ROUTE) 
  {
    if(dst.isValid())
    { 
      switch(dst.type)
      {
        case Route::TRACK_ROUTE:
          src.channel = dst.channel;
          return dst.track->inRoutes()->removeRoute(src);
        break;
        case Route::MIDI_DEVICE_ROUTE:
          return dst.device->inRoutes()->removeRoute(src);
        break;
        case Route::MIDI_PORT_ROUTE:
          return MusEGlobal::midiPorts[dst.midiPort].inRoutes()->removeRoute(src);
        break;
        case Route::JACK_ROUTE:
          // Do nothing - it's a direct Jack disconnection!
          return false;
        break;
      }
    }
    return false;
    
  }
  else if(dst.type == Route::JACK_ROUTE) 
  {
    if(src.isValid())
    { 
      switch(src.type)
      {
        case Route::TRACK_ROUTE:
          dst.channel = src.channel;
          return src.track->outRoutes()->removeRoute(dst);
        break;
        case Route::MIDI_DEVICE_ROUTE:
          return src.device->outRoutes()->removeRoute(dst);
        break;
        case Route::MIDI_PORT_ROUTE:
          return MusEGlobal::midiPorts[src.midiPort].outRoutes()->removeRoute(dst);
        break;
        case Route::JACK_ROUTE:
          // Do nothing - it's a direct Jack disconnection!
          return false;
        break;
      }
    }
    return false;
    
  }
  else if(src.type == Route::MIDI_PORT_ROUTE)  
  {
    bool ret1 = false;
    bool ret2 = false;
    if(src.isValid())
            ret1 = MusEGlobal::midiPorts[src.midiPort].outRoutes()->removeRoute(dst);
    if(dst.isValid())
    { 
      switch(dst.type)
      {
        case Route::TRACK_ROUTE:
          ret2 = dst.track->inRoutes()->removeRoute(src);
        break;
        case Route::MIDI_DEVICE_ROUTE:
          ret2 = dst.device->inRoutes()->removeRoute(src);
        break;
        case Route::MIDI_PORT_ROUTE:
          ret2 = MusEGlobal::midiPorts[dst.midiPort].inRoutes()->removeRoute(src);
        break;
        case Route::JACK_ROUTE:
          ret2 = false;
        break;
      }
    }
    return ret1 || ret2;
  }      
  else if(dst.type == Route::MIDI_PORT_ROUTE)  
  {
    bool ret1 = false;
    bool ret2 = false;
    if(src.isValid())
    { 
      switch(src.type)
      {
        case Route::TRACK_ROUTE:
          ret2 = src.track->outRoutes()->removeRoute(dst);
        break;
        case Route::MIDI_DEVICE_ROUTE:
          ret2 = src.device->outRoutes()->removeRoute(dst);
        break;
        case Route::MIDI_PORT_ROUTE:
          ret2 = MusEGlobal::midiPorts[src.midiPort].outRoutes()->removeRoute(dst);
        break;
        case Route::JACK_ROUTE:
          ret2 = false;
        break;
      }
    }
    
    if(dst.isValid())
      ret1 = MusEGlobal::midiPorts[dst.midiPort].inRoutes()->removeRoute(src);
    
    return ret1 || ret2;
  }
  else 
  {
    if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
    {
      fprintf(stderr, "removeRoute: source and destination are not tracks\n");
      return false;
    }
    
    // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
    //      After:  src  TrackA, Channel  4, Remote Channel  2   dst: TrackB channel  2 Remote Channel  4
    //
    // Ex. (Handled above, not used here. For example only.) 
    //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
    //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
    src.remoteChannel = src.channel;
    dst.remoteChannel = dst.channel;
    const int src_chan = src.channel;
    src.channel = dst.channel;
    dst.channel = src_chan;
    
    // Is the source an Aux Track or else does it have Aux Tracks routed to it?
    // Update the destination track's aux ref count, and all tracks it is routed to.
    if(src.isValid() && dst.isValid() && src.track->outRoutes()->contains(dst) && dst.track->inRoutes()->contains(src))
    {
      if(src.track->auxRefCount())
        src.track->updateAuxRoute( -src.track->auxRefCount(), dst.track );
      else
      if(src.track->type() == Track::AUDIO_AUX)
        src.track->updateAuxRoute( -1, dst.track );
    }
    
    bool ret1 = false;
    bool ret2 = false;
    
    if(src.isValid())
    {
      RouteList* o_rl = src.track->outRoutes();
      MusECore::iRoute o_ir = o_rl->find(dst);
      if(o_ir != o_rl->end())
      {
        o_rl->erase(o_ir);
        ret1 = true;
      }
    }  
    else
      fprintf(stderr, "removeRoute: source is track but invalid\n");
    
    if(dst.isValid())
    {
      RouteList* i_rl = dst.track->inRoutes();
      MusECore::iRoute i_ir = i_rl->find(src);
      if(i_ir != i_rl->end())
      {
        i_rl->erase(i_ir);
        ret2 = true;
      }
    }  
    else
      fprintf(stderr, "removeRoute: destination is track but invalid\n");
    
    return ret1 || ret2;
  }
  
  return false;
}

//---------------------------------------------------------
//   removeAllRoutes
//   If src is valid, disconnects all output routes from it.
//   If dst is valid, disconnects all input routes to it.
//   src and dst Route are used SIMPLY because Route provides convenient way to 
//    specify the different pointer types (track, port, jack)
//   This routine will ONLY look at the pointer, not the channel or port etc...
//   So far it only works with MidiDevice <-> Jack.
//---------------------------------------------------------

void removeAllRoutes(Route src, Route dst)
{
    // TODO: Is the source an Aux Track or else does it have Aux Tracks routed to it?
    // Update the destination track's aux ref count, and all tracks it is routed to.
    /* if(src.isValid() && dst.isValid()) DELETETHIS 8
    {
      if(src.track->auxRefCount())
        dst.track->updateAuxStates( -src.track->auxRefCount() );
      else  
      if(src.track->type() == Track::TrackType::AUDIO_AUX))
        dst.track->updateAuxStates( -1 );
    }  */

    if(src.isValid())  
    {
      if(src.type == Route::MIDI_DEVICE_ROUTE)
        src.device->outRoutes()->clear();
      else
        fprintf(stderr, "removeAllRoutes: source is not midi device\n");
    }  
      
    if(dst.isValid())  
    {
      if(dst.type == Route::MIDI_DEVICE_ROUTE)
        dst.device->inRoutes()->clear();
      else
        fprintf(stderr, "removeAllRoutes: dest is not midi device\n");
    }  
}

//---------------------------------------------------------
//   track2name
//    create string name representation for audio node
//---------------------------------------------------------

static QString track2name(const Track* n)
      {
      if (n == nullptr)
            return QWidget::tr("None");
      return n->name();
      }

//---------------------------------------------------------
//   icon
//---------------------------------------------------------


QPixmap* Route::icon(bool isSource, bool isMidi) const
{
    // temporary hack until all icons are SVG
    static QPixmap* ankerIcon = new QPixmap(MusEGui::ankerSVGIcon->pixmap(QSize(18, 18)));

    switch(type)
    {
    case TRACK_ROUTE:
        if(track)
            return new QPixmap(track->icon()->pixmap(16,16));
        break;

    case JACK_ROUTE:
        if(isMidi)
            return isSource ? MusEGui::routesMidiInIcon : MusEGui::routesMidiOutIcon;
        else
            return isSource ? MusEGui::routesInIcon : MusEGui::routesOutIcon;

    case MIDI_DEVICE_ROUTE:
        break;

    case MIDI_PORT_ROUTE:
        return ankerIcon;
    }
    return nullptr;
}
      
//---------------------------------------------------------
//   name
//    create string name representation for audio node
//---------------------------------------------------------

QString Route::name(int preferred_name_or_alias) const
{
      if(type == MIDI_DEVICE_ROUTE) 
      {
        if(device)
          return device->name();
        return QWidget::tr("None");
      }
      else
      if(type == JACK_ROUTE) 
      {
        if(MusEGlobal::checkAudioDevice() && jackPort)
        {
          char s[ROUTE_PERSISTENT_NAME_SIZE];
          return QString(MusEGlobal::audioDevice->portName(jackPort, s, ROUTE_PERSISTENT_NAME_SIZE, preferred_name_or_alias));
        }
        return QString(persistentJackPortName);
        
      }
      else
      if(type == MIDI_PORT_ROUTE) 
      {
        return ROUTE_MIDIPORT_NAME_PREFIX + QString().setNum(midiPort);
      }
      else
        return track2name(track);
}

//---------------------------------------------------------
//   name
//    fill and return str char name representation for audio node
//---------------------------------------------------------
char* Route::name(char* str, int str_size, int preferred_name_or_alias) const
{
      if(type == MIDI_DEVICE_ROUTE) 
        return MusELib::strntcpy(str, device ? device->name().toLatin1().constData() : 0, str_size);
      else
      if(type == JACK_ROUTE) 
      {
        if(MusEGlobal::checkAudioDevice() && jackPort)
          return MusEGlobal::audioDevice->portName(jackPort, str, str_size, preferred_name_or_alias);
        return MusELib::strntcpy(str, persistentJackPortName, str_size);
      }
      else
      if(type == MIDI_PORT_ROUTE) 
      {
        return MusELib::strntcpy(str, (ROUTE_MIDIPORT_NAME_PREFIX + QString().setNum(midiPort)).toLatin1().constData(), str_size);
      }
      else
        return MusELib::strntcpy(str, track ? track->name().toLatin1().constData() : 0, str_size);

}

//---------------------------------------------------------
//   displayName
//    create string name representation for audio node
//---------------------------------------------------------

QString Route::displayName(int preferred_name_or_alias) const
{
      if(type == MIDI_DEVICE_ROUTE) 
      {
        if(device)
          return device->name();
        return QWidget::tr("None");
      }
      else
      if(type == JACK_ROUTE) 
      {
        if(MusEGlobal::checkAudioDevice() && jackPort)
        {
          char s[ROUTE_PERSISTENT_NAME_SIZE];
          return QString(MusEGlobal::audioDevice->portName(jackPort, s, ROUTE_PERSISTENT_NAME_SIZE, preferred_name_or_alias));
        }
        return QString(persistentJackPortName);
        
      }
      else
      if(type == MIDI_PORT_ROUTE) 
      {
        return ROUTE_MIDIPORT_NAME_PREFIX + QString().setNum(midiPort);
      }
      else
        return QString("%1:%2").arg(MusEGlobal::song->tracks()->index(track) + 1).arg(track2name(track));
}

//---------------------------------------------------------
//   name2route
//---------------------------------------------------------

Route name2route(const QString& rn, bool /*dst*/, int rtype)
{
  int channel = -1;
  QString s(rn);
  // Support old route style in med files. Obsolete.
  if (rn[0].isNumber() && rn[1]==':') 
  {
    channel = rn[0].toLatin1() - int('1');
    s = rn.mid(2);
  }
  
  if(rtype == -1)
  {  
      if(MusEGlobal::checkAudioDevice())
      {
        void* p = MusEGlobal::audioDevice->findPort(s.toLatin1().constData());
        if(p)
          return Route(p, channel);
      }
      
      TrackList* tl = MusEGlobal::song->tracks();
      for(iTrack i = tl->begin(); i != tl->end(); ++i) 
      {
        if((*i)->isMidiTrack())
        {
          MidiTrack* track = (MidiTrack*)*i;
          if(track->name() == s)
            return Route(track, channel);
        }
        else
        {  
          AudioTrack* track = (AudioTrack*)*i;
          if(track->name() == s)
            return Route(track, channel);
        }      
      }
      
      for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
      {
        if((*i)->name() == s)
            return Route(*i, channel);
      }
    
      if(s.left(ROUTE_MIDIPORT_NAME_PREFIX.length()) == ROUTE_MIDIPORT_NAME_PREFIX)
      {
        bool ok = false;
        int port = s.mid(ROUTE_MIDIPORT_NAME_PREFIX.length()).toInt(&ok);
        if(ok)
          return Route(port, channel);
      }
  }
  else
  {
      if(rtype == Route::TRACK_ROUTE)
      {  
        TrackList* tl = MusEGlobal::song->tracks();
        for(iTrack i = tl->begin(); i != tl->end(); ++i) 
        {
          if((*i)->isMidiTrack())
          {
            MidiTrack* track = (MidiTrack*)*i;
            if(track->name() == s)
              return Route(track, channel);
          }
          else
          {  
            AudioTrack* track = (AudioTrack*)*i;
            if(track->name() == s)
              return Route(track, channel);
          }      
        }
        return Route((Track*) 0, channel);
      }
      else
      // TODO Distinguish the device types
      if(rtype == Route::MIDI_DEVICE_ROUTE)
      {  
        for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
        {
          if((*i)->name() == s)
            return Route(*i, channel);
        }
        return Route((MidiDevice*) 0, channel);
      }
      else
      if(rtype == Route::JACK_ROUTE)
      {  
        if(MusEGlobal::checkAudioDevice())
        {
          // REMOVE Tim. Persistent routes. TODO FIXME: Use new cached jack port names.
          void* p = MusEGlobal::audioDevice->findPort(s.toLatin1().constData());
          if(p)
            return Route(p, channel);
        }      
        return Route((void*) 0, channel);
      }
      else
      if(rtype == Route::MIDI_PORT_ROUTE) 
      {  
        if(s.left(ROUTE_MIDIPORT_NAME_PREFIX.length()) == ROUTE_MIDIPORT_NAME_PREFIX)
        {
          bool ok = false;
          int port = s.mid(ROUTE_MIDIPORT_NAME_PREFIX.length()).toInt(&ok);
          if(ok)
            return Route(port, channel);
        }
        return Route((int) 0, channel);
      }
  }
  
  fprintf(stderr, "  name2route: <%s> not found\n", rn.toLatin1().constData());
  return Route((Track*) 0, channel);
}

//---------------------------------------------------------
//   routeCanConnect
//---------------------------------------------------------

bool routeCanConnect(const Route& src, const Route& dst)
{
      if(!src.isValid() || !dst.isValid())
        return false;
      
      if(src.type == Route::JACK_ROUTE) 
      {           
            if(!MusEGlobal::checkAudioDevice() || 
               !src.jackPort || 
               MusEGlobal::audioDevice->portDirection(src.jackPort) != MusECore::AudioDevice::OutputPort)
              return false;
        
            if(dst.type == Route::TRACK_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(src.jackPort) != MusECore::AudioDevice::AudioPort || 
                 dst.track->type() != Track::AUDIO_INPUT || 
                 dst.channel < 0) 
                return false;
              const Route v_src(src.type, src.midiPort, src.voidPointer, dst.channel, src.channels, src.channel, src.persistentJackPortName);
              return !dst.track->inRoutes()->contains(v_src);
            }  
            else if(dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(src.jackPort) != MusECore::AudioDevice::MidiPort || 
                 dst.device->deviceType() != MidiDevice::JACK_MIDI)
                return false;
              const Route v_src(src.type, src.midiPort, src.voidPointer, dst.channel, src.channels, src.channel, src.persistentJackPortName);
              return !dst.device->inRoutes()->contains(v_src);
            }  
            else if(dst.type == Route::JACK_ROUTE) 
            {
              // Allow direct Jack connections!
              return MusEGlobal::audioDevice && MusEGlobal::audioDevice->portsCanConnect(src.jackPort, dst.jackPort);
            }
            else
              return false;
      }
      else if(dst.type == Route::JACK_ROUTE) 
      {
            if(!MusEGlobal::checkAudioDevice() || 
               !dst.jackPort || 
               MusEGlobal::audioDevice->portDirection(dst.jackPort) != MusECore::AudioDevice::InputPort)
              return false;
        
            if(src.type == Route::TRACK_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(dst.jackPort) != MusECore::AudioDevice::AudioPort || 
                 src.track->type() != Track::AUDIO_OUTPUT || src.channel < 0) 
                return false;
              const Route v_dst(dst.type, dst.midiPort, dst.voidPointer, src.channel, dst.channels, -1, dst.persistentJackPortName);
              return !src.track->outRoutes()->contains(v_dst);
            }
            else 
            if(src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(dst.jackPort) != MusECore::AudioDevice::MidiPort ||
                 src.device->deviceType() != MidiDevice::JACK_MIDI)
                return false;
              const Route v_dst(dst.type, dst.midiPort, dst.voidPointer, src.channel, dst.channels, -1, dst.persistentJackPortName);
              return !src.device->outRoutes()->contains(v_dst);
            }
            else if(src.type == Route::JACK_ROUTE) 
            {
              // Allow direct Jack connections!
              return MusEGlobal::audioDevice && MusEGlobal::audioDevice->portsCanConnect(src.jackPort, dst.jackPort);
            }
            else
              return false;
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  
      {
            if(dst.type != Route::TRACK_ROUTE || !dst.track->isMidiTrack() || dst.channel < -1 || dst.channel >= MusECore::MUSE_MIDI_CHANNELS)
              return false;
            
            //MidiPort *mp = &MusEGlobal::midiPorts[src.midiPort];
            
            // Do not allow synth ports to connect to any track. It may be useful in some cases, 
            //  may be desired later, but for now it's just a routing hassle.  p4.0.35 
            //if(mp->device() && mp->device()->isSynti())
            //  return false;
            
            const Route v_src(src.type, src.midiPort, src.voidPointer, dst.channel, src.channels, src.channel, src.persistentJackPortName);
            // If one route node exists and one is missing, it's OK to reconnect, addRoute will take care of it.
            if(!MusEGlobal::midiPorts[src.midiPort].outRoutes()->contains(dst) || !dst.track->inRoutes()->contains(v_src))
              return true;
            
            return false;
      }
      else if(dst.type == Route::MIDI_PORT_ROUTE)  
      {
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            return false;
#else  
        
            if(src.type != Route::TRACK_ROUTE || !src.track->isMidiTrack() || src.channel < -1 || src.channel >= MusECore::MUSE_MIDI_CHANNELS)
              return false;
            const Route v_dst(dst.type, dst.midiPort, dst.voidPointer, src.channel, dst.channels, dst.channel, dst.persistentJackPortName);
            
            // If one route node exists and one is missing, it's OK to reconnect, addRoute will take care of it.
            if(!src.track->outRoutes()->contains(v_dst) || !MusEGlobal::midiPorts[dst.midiPort].inRoutes()->contains(src))
              return true;
            
            return false;
#endif  
      }
      else 
      {
        if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
          return false;
        if(src.track && dst.track && src.track == dst.track)
          return false;

        switch(src.track->type())
        {
          case Track::MIDI:
          case Track::DRUM:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_AUX:
              case Track::AUDIO_SOFTSYNTH:
                return false;
                
              case Track::AUDIO_INPUT:
                if(src.channel >= 0)
                  return false;
              break;
            }
          break;
            
            
          case Track::AUDIO_OUTPUT:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_AUX:
              case Track::AUDIO_SOFTSYNTH:
                return false;
                
              case Track::AUDIO_INPUT:
                if(src.channel >= 0 || dst.channel >= 0)
                  return false;
              break;
            }
          break;
            
          case Track::AUDIO_INPUT:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::WAVE:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::AUDIO_GROUP:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::AUDIO_AUX:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::AUDIO_GROUP:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::AUDIO_SOFTSYNTH:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::AUDIO_GROUP:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
        }
        
        if((src.channel == -1 && dst.channel != -1) || (dst.channel == -1 && src.channel != -1))
          return false;

        if(src.channels != dst.channels)
          return false;
        
        // Allow for -1 = omni route.
        if(src.channel >= src.track->routeCapabilities()._trackChannels._outChannels ||
           dst.channel >= dst.track->routeCapabilities()._trackChannels._inChannels)
          return false;
          
        if(src.track->isCircularRoute(dst.track)) 
          return false;
      
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src  TrackA, Channel  4, Remote Channel  2   dst: TrackB channel  2 Remote Channel  4
        //
        // Ex. (Handled above, not used here. For example only.) 
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        const Route v_src(src.type, src.midiPort, src.voidPointer, dst.channel, src.channels, src.channel, src.persistentJackPortName);
        const Route v_dst(dst.type, dst.midiPort, dst.voidPointer, src.channel, dst.channels, dst.channel, dst.persistentJackPortName);
           
        // Allow it to reconnect a partial route.
        if(!v_src.isValid() || !v_dst.isValid() || (src.track->outRoutes()->contains(v_dst) && dst.track->inRoutes()->contains(v_src)))
          return false;
        
        return true;
      }
      return false;
}

//---------------------------------------------------------
//   routeCanDisconnect
//---------------------------------------------------------

bool routeCanDisconnect(const Route& src, const Route& dst)
{
      if(src.type == Route::JACK_ROUTE) 
      {
            //if(!dst.isValid())
            if(!dst.exists())
              return false;
            
            if(dst.type == Route::TRACK_ROUTE) 
            {
              if(dst.track->type() != Track::AUDIO_INPUT) 
                return false;
              const Route v_src(src.type, src.midiPort, src.voidPointer, dst.channel, src.channels, -1, src.persistentJackPortName);
              return dst.track->inRoutes()->contains(v_src);
            }  
            else if(dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              return dst.device->inRoutes()->contains(src);
            }  
            else if(dst.type == Route::JACK_ROUTE) 
            {
              // Allow direct Jack connections! Pass the port names here instead of ports so that 
              //  persistent routes (where jackPort = NULL) can be removed.
              return MusEGlobal::audioDevice && MusEGlobal::audioDevice->portsCanDisconnect(src.persistentJackPortName, dst.persistentJackPortName);
            }
            else
              return false;
      }
      else if(dst.type == Route::JACK_ROUTE) 
      {
            if(!src.exists())
              return false;
            
            if(src.type == Route::TRACK_ROUTE) 
            {
              if(src.track->type() != Track::AUDIO_OUTPUT) 
                    return false;
              const Route v_dst(dst.type, dst.midiPort, dst.voidPointer, src.channel, dst.channels, -1, dst.persistentJackPortName);
              return src.track->outRoutes()->contains(v_dst);
            }  
            else if(src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              return src.device->outRoutes()->contains(dst);
            }  
            else if(src.type == Route::JACK_ROUTE) 
            {
              // Allow direct Jack disconnections! Pass the port names here instead of ports so that 
              //  persistent routes (where jackPort = NULL) can be removed.
              return MusEGlobal::audioDevice && MusEGlobal::audioDevice->portsCanDisconnect(src.persistentJackPortName, dst.persistentJackPortName);
            }
            else
              return false;
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  
      {
        if(!src.isValid() || src.channel < -1 || src.channel >= MusECore::MUSE_MIDI_CHANNELS ||
           dst.type != Route::TRACK_ROUTE || !dst.exists() || !dst.track->isMidiTrack() || dst.channel < -1 || dst.channel >= MusECore::MUSE_MIDI_CHANNELS)
          return false;
        
        // Allow it to disconnect a partial route.
        if(MusEGlobal::midiPorts[src.midiPort].outRoutes()->contains(dst) || dst.track->inRoutes()->contains(src))
          return true;
        
        return false;
      }      
      else if(dst.type == Route::MIDI_PORT_ROUTE)  
      {
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        return false;
#else        
        
        if(!dst.isValid() || dst.channel < -1 || dst.channel >= MusECore::MUSE_MIDI_CHANNELS ||
           src.type != Route::TRACK_ROUTE || !src.exists() || !src.track->isMidiTrack() || src.channel < -1 || src.channel >= MusECore::MUSE_MIDI_CHANNELS)
          return false;

        // Allow it to disconnect a partial route.
        if(src.track->outRoutes()->contains(dst) || MusEGlobal::midiPorts[dst.midiPort].inRoutes()->contains(src))
          return true;
        
        return false;
#endif        
      }
      else 
      {
            if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
              return false;
            if(src.track && dst.track && src.track == dst.track)
              return false;

            // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
            //      After:  src  TrackA, Channel  4, Remote Channel  2   dst: TrackB channel  2 Remote Channel  4
            //
            // Ex. (Handled above, not used here. For example only.) 
            //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
            //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
            const Route v_src(src.type, src.midiPort, src.voidPointer, dst.channel, src.channels, src.channel, src.persistentJackPortName);
            const Route v_dst(dst.type, dst.midiPort, dst.voidPointer, src.channel, dst.channels, dst.channel, dst.persistentJackPortName);
            
            // Allow it to disconnect a partial route.
            if((v_src.exists() && src.track->outRoutes()->contains(v_dst)) || (v_dst.exists() && dst.track->inRoutes()->contains(v_src)))
              return true;
            
            return false;
      }
      return false;
}

//---------------------------------------------------------
//   routeConnectionPossible
//---------------------------------------------------------

bool routesCompatible(const Route& src, const Route& dst, bool check_types_only)
{
      if(!src.isValid() || !dst.isValid())
        return false;
      
      if(src.type == Route::JACK_ROUTE) 
      {     
            if(!MusEGlobal::checkAudioDevice() || 
               !src.jackPort || 
               MusEGlobal::audioDevice->portDirection(src.jackPort) != MusECore::AudioDevice::OutputPort)
              return false;
        
            if(dst.type == Route::TRACK_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(src.jackPort) != MusECore::AudioDevice::AudioPort || 
                 dst.track->type() != Track::AUDIO_INPUT || 
                 (!check_types_only && dst.channel < 0))
                return false;
              return true;
            }  
            else if(dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(src.jackPort) != MusECore::AudioDevice::MidiPort || 
                 dst.device->deviceType() != MidiDevice::JACK_MIDI)
                return false;
              return true;
            }  
            else if(dst.type == Route::JACK_ROUTE) 
              // Allow direct Jack connections!
              return MusEGlobal::audioDevice->portsCompatible(src.jackPort, dst.jackPort);
            else
              return false;
      }
      else if(dst.type == Route::JACK_ROUTE) 
      {
            if(!MusEGlobal::checkAudioDevice() || 
               !dst.jackPort || 
               MusEGlobal::audioDevice->portDirection(dst.jackPort) != MusECore::AudioDevice::InputPort)
              return false;
        
            if(src.type == Route::TRACK_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(dst.jackPort) != MusECore::AudioDevice::AudioPort || 
                 src.track->type() != Track::AUDIO_OUTPUT ||
                 (!check_types_only && src.channel < 0))
                return false;
              return true;
            }
            else if(src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(MusEGlobal::audioDevice->portType(dst.jackPort) != MusECore::AudioDevice::MidiPort ||
                 src.device->deviceType() != MidiDevice::JACK_MIDI)
                return false;
              return true;
            }
            // Unnecessary. This condition is handled above.
            //else if(src.type == Route::JACK_ROUTE) 
            //  // Allow direct Jack connections!
            //  return MusEGlobal::audioDevice->portsCompatible(src.jackPort, dst.jackPort);
            else
              return false;
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  
      {
            if(dst.type != Route::TRACK_ROUTE || !dst.track->isMidiTrack())
              return false;
            
            //MidiPort *mp = &MusEGlobal::midiPorts[src.midiPort];
            
            // Do not allow synth ports to connect to any track. It may be useful in some cases, 
            //  may be desired later, but for now it's just a routing hassle.  p4.0.35 
            //if(mp->device() && mp->device()->isSynti())
            //  return false;
            
            if(check_types_only)
              return true;
            
            if(dst.channel < -1 || dst.channel >= MusECore::MUSE_MIDI_CHANNELS)
              return false;
                        
            // If one route node exists and one is missing, it's OK to reconnect, addRoute will take care of it.
              return true;
      }
      else if(dst.type == Route::MIDI_PORT_ROUTE)  
      {
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            return false;
#else      
            
            if(src.type != Route::TRACK_ROUTE || !src.track->isMidiTrack())
              return false;
            
            if(check_types_only)
              return true;

            if(src.channel < -1 || src.channel >= MusECore::MUSE_MIDI_CHANNELS)
              return false;
            
            // If one route node exists and one is missing, it's OK to reconnect, addRoute will take care of it.
              return true;
#endif      
      }
      else 
      {
        if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
          return false;
        if(src.track && dst.track && src.track == dst.track)
          return false;

        switch(src.track->type())
        {
          case Track::MIDI:
          case Track::DRUM:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_AUX:
              case Track::AUDIO_SOFTSYNTH:
                return false;
                
              case Track::AUDIO_INPUT:
                if(!check_types_only && src.channel >= 0)
                  return false;
              break;
            }
          break;
            
            
          case Track::AUDIO_OUTPUT:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_AUX:
              case Track::AUDIO_SOFTSYNTH:
                return false;
                
              case Track::AUDIO_INPUT:
                if(!check_types_only && (src.channel >= 0 || dst.channel >= 0))
                  return false;
              break;
            }
          break;
            
          case Track::AUDIO_INPUT:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::WAVE:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::AUDIO_GROUP:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_GROUP:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::AUDIO_AUX:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::AUDIO_GROUP:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
          case Track::AUDIO_SOFTSYNTH:
            switch(dst.track->type())
            {
              case Track::MIDI:
              case Track::DRUM:
              case Track::AUDIO_INPUT:
              case Track::AUDIO_AUX:
                return false;
                
              case Track::AUDIO_GROUP:
              case Track::WAVE:
              case Track::AUDIO_OUTPUT:
              case Track::AUDIO_SOFTSYNTH:
              break;
            }
          break;
          
        }

        // Don't bother with the circular route check, below.
        if(check_types_only)
          return true;
        
        if((src.channel == -1 && dst.channel != -1) || 
           (dst.channel == -1 && src.channel != -1) || 
           (src.channels != dst.channels))
          return false;

        // Allow for -1 = omni route.
        if(src.channel >= src.track->routeCapabilities()._trackChannels._outChannels ||
           dst.channel >= dst.track->routeCapabilities()._trackChannels._inChannels ||
           src.track->isCircularRoute(dst.track))
          return false;
          
        return true;
      }
      return false;
}


//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Route::read(Xml& xml)
{
      QString s;
      int dtype = MidiDevice::ALSA_MIDI;
      int port = -1;
      int track_idx = -1;
      RouteType rtype = Route::TRACK_ROUTE;
      
      for (;;) 
      {
            const QString& tag = xml.s1();
            Xml::Token token = xml.parse();
            switch (token) 
            {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        #ifdef ROUTE_DEBUG
                        fprintf(stderr, "Route::read(): attribute:%s\n", tag.toLatin1().constData());
                        #endif
                        if(tag == "type")
                          rtype = RouteType(xml.s2().toInt());
                        else
                        if(tag == "devtype")
                        {
                          dtype = xml.s2().toInt();
                          rtype = Route::MIDI_DEVICE_ROUTE;
                        }
                        else
                        if(tag == "name")
                          s = xml.s2();
                        else
                        if(tag == "track") 
                          track_idx = xml.s2().toInt();
                        else
                        if(tag == "mport") 
                        {
                          port = xml.s2().toInt();
                          rtype = Route::MIDI_PORT_ROUTE;
                        }
                        else  
                          fprintf(stderr, "Route::read(): unknown attribute:%s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        #ifdef ROUTE_DEBUG
                        fprintf(stderr, "Route::read(): tag end type:%d channel:%d name:%s\n", rtype, channel, s.toLatin1().constData());
                        #endif
                        if(rtype == MIDI_PORT_ROUTE)  
                        {
                          if(port >= 0 && port < MusECore::MIDI_PORTS)
                          {
                            type = rtype;
                            midiPort = port;
                          }
                          else
                            fprintf(stderr, "Route::read(): midi port <%d> out of range\n", port);
                        }
                        else
                        // New track index method replaces obsolete track name method below.
                        if(track_idx >= 0)
                        {
                          if(rtype == TRACK_ROUTE) 
                          {
                            const TrackList* tl = MusEGlobal::song->tracks();
                            Track* t = tl->index(track_idx);
                            if(t)
                            {
                              track = t;
                              type = rtype;
                            }
                            else
                              fprintf(stderr, "Route::read(): track index <%d> not found\n", track_idx);
                          }
                        }
                        else
                        if(!s.isEmpty())
                        {
                          // OBSOLETE. Keep for backwards compatibility. Replaced by track index method above.
                          if(rtype == TRACK_ROUTE) 
                          {
                            TrackList* tl = MusEGlobal::song->tracks();
                            iTrack i = tl->begin();
                            for ( ; i != tl->end(); ++i) 
                            {
                              Track* t = *i;
                              if (t->name() == s) 
                              {
                                track = t;
                                type = rtype;
                                break;
                              }
                            }
                            if(i == tl->end())
                              fprintf(stderr, "Route::read(): track <%s> not found\n", s.toLocal8Bit().constData());
                          }
                          else
                          if(rtype == JACK_ROUTE) 
                          {
                            type = rtype;
                            jackPort = 0;
                            if(MusEGlobal::audioDevice) // fix crash if jack is zombified at this point
                            {
                              jackPort = MusEGlobal::audioDevice->findPort(s.toLatin1().constData());
                              if(jackPort)
                                // Replace the name with a more appropriate one at this time.
                                MusEGlobal::audioDevice->portName(jackPort, persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
                            }
                            // The graph change handler will replace persistentJackPortName with a more appropriate name if necessary.
                            if(!jackPort)
                              MusELib::strntcpy(persistentJackPortName, s.toLatin1().constData(), ROUTE_PERSISTENT_NAME_SIZE);
                          }
                          else
                          if(rtype == MIDI_DEVICE_ROUTE)
                          {
                            iMidiDevice imd = MusEGlobal::midiDevices.begin();
                            for( ; imd != MusEGlobal::midiDevices.end(); ++imd) 
                            {
                              MidiDevice* md = *imd;
                              if(md->name() == s && md->deviceType() == dtype) 
                              {
                                // We found a device, but if it is not in use by the song (port is -1), ignore it. 
                                // This prevents loading and propagation of bogus routes in the med file.
                                // We found a device, but if it is not a jack midi and in use by the song (port is -1), ignore it. 
                                // This prevents loading and propagation of bogus routes in the med file.
                                if(md->midiPort() == -1 && md->deviceType() != MidiDevice::JACK_MIDI)
                                  break;
                                
                                device = md;
                                type = rtype;
                                break;
                              }
                            }
                            if(imd == MusEGlobal::midiDevices.end())
                              fprintf(stderr, "Route::read(): midi device <%s> not found\n", s.toLatin1().constData());
                          }
                        }
                        return;
                  default:
                        break;
            }
      }
}


//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::readRoute(Xml& xml)
{
      QString src;
      QString dst;
      int ch          = -1;
      int chmask      = -1;
      int chs         = -1;
      int remch       = -1;
      bool midi_track_out_set = false;

      Route sroute, droute;
      
      for (;;) 
      {
            const QString& tag = xml.s1();
            Xml::Token token = xml.parse();
            switch (token) 
            {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        // 2010/02/03 Support old routes in med files. Now obsolete!
                        if (tag == "srcNode")
                              src = xml.parse1();
                        else if (tag == "dstNode")
                              dst = xml.parse1();
                        // Support new routes.
                        else if (tag == "source")
                        {
                              sroute.read(xml);
                              sroute.channel       = ch;
                              sroute.channels      = chs;
                              sroute.remoteChannel = remch;
                        }
                        else if (tag == "dest")
                        {
                              droute.read(xml);
                              droute.channels      = chs;
                              // If channels was given, it should be a multi-channel audio route.
                              // Convert to new scheme by switching them around for the destination route:
                              if(chs > 0)
                              {
                                droute.channel       = remch;
                                droute.remoteChannel = ch;
                              }
                              else
                              {
                                droute.channel       = ch;
                                droute.remoteChannel = remch;
                              }
                        }      
                        else
                              xml.unknown("readRoute");
                        break;
                  case Xml::Attribut:
                        #ifdef ROUTE_DEBUG
                        fprintf(stderr, "Song::readRoute(): attribute:%s\n", tag.toLatin1().constData());
                        #endif
                        if(tag == "channel")
                          ch = xml.s2().toInt();
                        else
                        if(tag == "channels")
                          chs = xml.s2().toInt();
                        else
                        if(tag == "remch")
                          remch = xml.s2().toInt();
                        else
                        if(tag == "channelMask")           // New channel mask for midi port-track routes.
                          chmask = xml.s2().toInt();               
                        else
                          fprintf(stderr, "Song::readRoute(): unknown attribute:%s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "Route") 
                        {
                          // Support old routes in med files. Now obsolete!
                          if(!src.isEmpty() && !dst.isEmpty())
                          {
                            Route s = name2route(src, false);
                            Route d = name2route(dst, true);
                            addRoute(s, d);
                          }  
                          else
                          // Support new routes.
                          if(sroute.isValid() && droute.isValid())
                          {    
                            // Support pre- 1.1-RC2 midi device to track routes. Obsolete. Replaced with midi port routes.
                            if(sroute.type == Route::MIDI_DEVICE_ROUTE && droute.type == Route::TRACK_ROUTE) 
                            {
                              if(sroute.device->midiPort() >= 0 && sroute.device->midiPort() < MusECore::MIDI_PORTS
                                 && ch >= 0 && ch < MusECore::MUSE_MIDI_CHANNELS)         
                              {
                                sroute.midiPort = sroute.device->midiPort();
                                sroute.device = 0;
                                sroute.type = Route::MIDI_PORT_ROUTE;
                                
                                sroute.channel = ch;
                                droute.channel = sroute.channel;
                                
                                addRoute(sroute, droute);
                              }
                              else  
                                fprintf(stderr, "  Warning - device:%s to track route, no device midi port or chan:%d out of range. Ignoring route!\n", 
                                       sroute.device->name().toLatin1().constData(), ch);
                            }
                            // Support pre- 1.1-RC2 track to midi device routes. Obsolete. Replaced with midi port routes.
                            else if(sroute.type == Route::TRACK_ROUTE && droute.type == Route::MIDI_DEVICE_ROUTE)
                            {
                              // Device and track already validated in ::read().
                              const int port = droute.device->midiPort();
                              if(port >= 0 && port < MusECore::MIDI_PORTS
                                 && ch >= 0 && ch < MusECore::MUSE_MIDI_CHANNELS &&
                                 sroute.track->isMidiTrack())
                              {
                                MidiTrack* mt = static_cast<MidiTrack*>(sroute.track);
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                                if(!midi_track_out_set)
                                {
                                  midi_track_out_set = true;
                                  MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
                                  MusEGlobal::audio->msgIdle(true);
                                  changed |= mt->setOutPortAndChannelAndUpdate(port, ch, false);
                                  MusEGlobal::audio->msgIdle(false);
                                  MusEGlobal::audio->msgUpdateSoloStates();
                                  MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
                                }
#else
                                droute.midiPort = droute.device->midiPort();
                                droute.device = 0;
                                droute.type = Route::MIDI_PORT_ROUTE;
                                droute.channel = ch;
                                sroute.channel = droute.channel;
                                addRoute(sroute, droute);
#endif
                              }  
                              else  
                                fprintf(stderr, "  Warning - track to device:%s route, no device midi port or chan:%d out of range. Ignoring route!\n", 
                                       droute.device->name().toLatin1().constData(), ch);
                            }
                            // Support old bit-wise channel mask for midi port to midi track routes and midi port to audio input soling chain routes. Obsolete!
                            // Check for song file version 2.0 or below:
                            else if(chmask > 0 && (xml.majorVersion() * 1000000 + xml.minorVersion()) <= 2000000)  // Arbitrary shift, and add
                            {
                              fprintf(stderr, "  Warning - Route: Converting old single-route bitwise channel mask:%d to individual routes\n", chmask);

                              if(sroute.type == Route::MIDI_PORT_ROUTE && droute.type == Route::TRACK_ROUTE)
                              {
                                if(droute.track->isMidiTrack())
                                {
                                  // All channels set? Convert to new Omni route.
                                  if(chmask == ((1 << MusECore::MUSE_MIDI_CHANNELS) - 1))
                                  {
                                    sroute.channel = -1;
                                    droute.channel = -1;
                                    addRoute(sroute, droute);
                                  }
                                  else
                                  {
                                    // Check each channel bit:
                                    for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
                                    {
                                      const int chbit = 1 << i;
                                      // Is channel bit set?
                                      if(chmask & chbit)
                                      {
                                        // Convert to individual routes:
                                        sroute.channel = i;
                                        droute.channel = i;
                                        addRoute(sroute, droute);
                                      }
                                    }
                                  }                                  
                                }
                                // Support old midi port to audio input soloing chain routes. Obsolete!
                                else if(droute.track->type() == Track::AUDIO_INPUT)
                                {
                                  const int port = sroute.midiPort;
                                  // Check each channel bit:
                                  for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
                                  {
                                    const int chbit = 1 << i;
                                    // Is channel bit set?
                                    if(chmask & chbit)
                                    {
                                      const MusECore::MidiTrackList* const mtl = MusEGlobal::song->midis();
                                      for(ciMidiTrack imt = mtl->begin(); imt != mtl->end(); ++imt)
                                      {
                                        MidiTrack* const mt = *imt;
                                        if(mt->outPort() == port && mt->outChannel() == i)
                                        {
                                          // Convert to a midi track to audio input route:
                                          sroute.type = Route::TRACK_ROUTE;
                                          sroute.track = mt;
                                          sroute.midiPort = -1;
                                          sroute.channel = sroute.channels = sroute.remoteChannel = droute.channel = droute.channels = droute.remoteChannel = -1;
                                          addRoute(sroute, droute);
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                            // If channels was given, it must be a multi-channel audio route:
                            else if(chs > 0)
                            {
                              // If EITHER the channel or the remote channel are zero but the other not given, convert to an Omni route:
                              if((ch == -1 && remch == -1) || (ch == 0 && remch == -1) || (remch == 0 && ch == -1))
                              {
                                sroute.channel = sroute.remoteChannel = sroute.channels = droute.channel = droute.remoteChannel = droute.channels = -1;
                                addRoute(sroute, droute);
                              }
                              // Otherwise convert to individual routes: 
                              else
                              {
                                sroute.channels = droute.channels = 1;
                                if(sroute.channel == -1)
                                  sroute.channel = 0;
                                if(sroute.remoteChannel == -1)
                                  sroute.remoteChannel = 0;
                                if(droute.channel == -1)
                                  droute.channel = 0;
                                if(droute.remoteChannel == -1)
                                  droute.remoteChannel = 0;
                                for(int i = 0; i < chs; ++i)
                                {
                                  addRoute(sroute, droute);
                                  ++sroute.channel;
                                  ++sroute.remoteChannel;
                                  ++droute.channel;
                                  ++droute.remoteChannel;
                                }
                              }
                            }  
                            else
                               
                              addRoute(sroute, droute);
                          }
                          else
                            fprintf(stderr, "  Warning - route invalid. Ignoring route!\n");
                          
                          return;
                        }
                  default:
                        break;
             }
      }
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Route::dump() const
{
      if (type == TRACK_ROUTE)
      {
        if(track)
          fprintf(stderr, "Route dump: track <%s> channel %d channels %d\n", track->name().toLocal8Bit().constData(), channel, channels);
      }      
      else 
      if (type == JACK_ROUTE)
      {
        if(MusEGlobal::checkAudioDevice())
        {
          if(jackPort)
          {
            char s[ROUTE_PERSISTENT_NAME_SIZE];
            fprintf(stderr, "Route dump: jack audio port %p <%s> persistent name <%s> channel %d\n", jackPort, MusEGlobal::audioDevice->portName(jackPort, s, ROUTE_PERSISTENT_NAME_SIZE), persistentJackPortName, channel);
          }
          else
            fprintf(stderr, "Route dump: jack audio port %p persistent name <%s> channel %d\n", jackPort, persistentJackPortName, channel);
        }
      }
      else 
      if (type == MIDI_PORT_ROUTE) 
      {
        fprintf(stderr, "Route dump: midi port <%d> channel mask %d\n", midiPort, channel);
      }
      else
      if (type == MIDI_DEVICE_ROUTE)
      {
        fprintf(stderr, "Route dump: ");
        if(device)
        {
          if(device->deviceType() == MidiDevice::JACK_MIDI)
          {
            if(MusEGlobal::checkAudioDevice())
            {  
              fprintf(stderr, "jack midi device <%s> ", device->name().toLatin1().constData());
              if(device->inClientPort())
              {
                char s[ROUTE_PERSISTENT_NAME_SIZE];
                fprintf(stderr, "input port <%s> ", 
                       //MusEGlobal::audioDevice->portName(device->inClientPort()).toLatin1().constData());
                       MusEGlobal::audioDevice->portName(device->inClientPort(), s, ROUTE_PERSISTENT_NAME_SIZE));
              }
              if(device->outClientPort())
              {
                char s[ROUTE_PERSISTENT_NAME_SIZE];
                fprintf(stderr, "output port <%s> ", 
                       //MusEGlobal::audioDevice->portName(device->outClientPort()).toLatin1().constData());
                       MusEGlobal::audioDevice->portName(device->outClientPort(), s, ROUTE_PERSISTENT_NAME_SIZE));
              }
            }           
          }
          else
          if(device->deviceType() == MidiDevice::ALSA_MIDI)
            fprintf(stderr, "alsa midi device <%s> ", device->name().toLatin1().constData());
          else
          if(device->deviceType() == MidiDevice::SYNTH_MIDI)
            fprintf(stderr, "synth midi device <%s> ", device->name().toLatin1().constData());
          else
            fprintf(stderr, "is midi but unknown device type:%d, ", device->deviceType());
        }
        else
          fprintf(stderr, "is midi but invalid device, ");
          
        fprintf(stderr, "channel:%d\n", channel);
      }
      else
        fprintf(stderr, "Route dump: unknown route type:%d\n", type);
}

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool Route::operator==(const Route& a) const
{
      if ((type == a.type) && (channel == a.channel)) 
      {
            if (type == TRACK_ROUTE)
            {
                  return track == a.track && channels == a.channels && remoteChannel == a.remoteChannel;
            }
            else 
            {
              if (type == JACK_ROUTE)
              {
                    // If the ports are valid compare them, otherwise compare the persistent port names.
                    if(jackPort && a.jackPort)
                      return jackPort == a.jackPort;    // Simplified.
                    else
                      return strcmp(persistentJackPortName, a.persistentJackPortName) == 0;
              }
              else 
              if (type == MIDI_PORT_ROUTE) 
              {
                return midiPort == a.midiPort;
              }
              else 
              if (type == MIDI_DEVICE_ROUTE)
              {
                return device == a.device;
              }
            }    
      }
      return false;
}

//---------------------------------------------------------
//   exists
//---------------------------------------------------------

bool Route::exists() const
{
  switch(type)
  {
    case MusECore::Route::TRACK_ROUTE:
      return MusEGlobal::song->tracks()->contains(track);
    break;
    
    case MusECore::Route::JACK_ROUTE:
      return MusEGlobal::checkAudioDevice() && MusEGlobal::audioDevice->findPort(persistentJackPortName);
    break;
    
    case MusECore::Route::MIDI_DEVICE_ROUTE:
      return MusEGlobal::midiDevices.contains(device);
    break;
      
    case MusECore::Route::MIDI_PORT_ROUTE:
      return isValid();
    break;
  }
  return false;
}

//---------------------------------------------------------
//   compare
//---------------------------------------------------------

bool Route::compare(const Route& a) const
{
      //if ((type == a.type) && (channel == a.channel)) 
      if (type == a.type) 
      {
            if (type == TRACK_ROUTE)
            {
                  return track == a.track && 
                         channels == a.channels && 
                         ((a.channel == -1) ? (channel == -1) : (channel != -1)) && 
                         //remoteChannel == a.remoteChannel;
                         ((a.remoteChannel == -1) ? (remoteChannel == -1) : (remoteChannel != -1));  // TODO: Want this? Seems logical.
            }
            else 
            //if(channel == a.channel)
            {
              if (type == JACK_ROUTE && channel == a.channel)
              {
                    // If the ports are valid compare them, otherwise compare the persistent port names.
                    if(jackPort && a.jackPort)
                      return jackPort == a.jackPort;    // Simplified.
                    else
                      return strcmp(persistentJackPortName, a.persistentJackPortName) == 0;
              }
              else 
              if (type == MIDI_PORT_ROUTE) 
              {
                return midiPort == a.midiPort;
              }
              else 
              if (type == MIDI_DEVICE_ROUTE)
              {
                return device == a.device;
              }
            }    
      }
      return false;
}

/* Please keep this just in case...
//---------------------------------------------------------
//   isCircularRoute
//   Recursive.
//   If dst is valid, start traversal from there, not from src.
//   Returns true if circular.
//---------------------------------------------------------

bool isCircularRoutePath(Track* src, Track* dst)
{
  bool rv = false;
  
  if(dst)
  {  
    src->setNodeTraversed(true);
    rv = isCircularRoutePath(dst, NULL);
    src->setNodeTraversed(false);
    return rv;
  }
  
  if(src->nodeTraversed())
    return true;
  
  src->setNodeTraversed(true);
  
  RouteList* orl = src->outRoutes();
  for (iRoute i = orl->begin(); i != orl->end(); ++i) 
  {
    if( !(*i).isValid() || (*i).type != Route::TRACK_ROUTE )
      continue;
    Track* t = (*i).track;
    //if(t->isMidiTrack())
    //  continue;
    rv = isCircularRoutePath(src, NULL);
    if(rv)
      break; 
  }
  
  src->setNodeTraversed(false);
  return rv;
}
*/


} // namespace MusECore
