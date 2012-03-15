//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.cpp,v 1.18.2.3 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
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

#include <QWidget>

#include "song.h"
#include "route.h"
#include "node.h"
#include "audio.h"
#include "track.h"
#include "synth.h"
#include "audiodev.h"
#include "xml.h"
#include "midiport.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"

//#define ROUTE_DEBUG 

namespace MusECore {

const QString ROUTE_MIDIPORT_NAME_PREFIX = "MusE MidiPort ";

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

Route::Route(void* t, int ch)
      {
      jackPort = t;
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
      }

Route::Route(MidiDevice* d, int ch)
{
      device   = d;  
      midiPort = -1;
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      type    = MIDI_DEVICE_ROUTE; 
}

Route::Route(int port, int ch)  
{
      track    = 0;
      midiPort = port; 
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      type    = MIDI_PORT_ROUTE;     
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
      if(type == TRACK_ROUTE)
      {
        track = node.track;
        midiPort = -1;
      }
      else
      if(type == JACK_ROUTE)
      {  
        jackPort = node.jackPort;
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
      }

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void addRoute(Route src, Route dst)
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
            return;
      }
      
//      printf("addRoute %d.%d:<%s> %d.%d:<%s>\n",
//         src.type, src.channel, src.name().toLatin1().constData(),
//         dst.type, dst.channel, dst.name().toLatin1().constData());
      if (src.type == Route::JACK_ROUTE) 
      {           
            if (dst.type == Route::TRACK_ROUTE) 
            {
              if (dst.track->type() != Track::AUDIO_INPUT) 
              {
                  fprintf(stderr, "addRoute: source is jack, dest:%s is track but not audio input\n", dst.track->name().toLatin1().constData());
                  return;
              }
              if (dst.channel < 0) 
              {
                  fprintf(stderr, "addRoute: source is jack, dest:%s is track but invalid channel:%d\n", dst.track->name().toLatin1().constData(), dst.channel);
                  return;
              }
              
              src.channel = dst.channel;
              RouteList* inRoutes = dst.track->inRoutes();
              for (ciRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
              {
                if (*i == src)    // route already there
                {
                  fprintf(stderr, "addRoute: src track route already exists.\n");
                  return;
                }  
              }
              #ifdef ROUTE_DEBUG
              fprintf(stderr, "addRoute: src Jack dst track name: %s pushing source route\n", dst.track->name().toLatin1().constData());
              #endif
              inRoutes->push_back(src);
            }  
            else
            if (dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(dst.device->deviceType() == MidiDevice::JACK_MIDI)
              {
                src.channel = dst.channel;
                RouteList* routes = dst.device->inRoutes();
                for (ciRoute i = routes->begin(); i != routes->end(); ++i) 
                {
                  if (*i == src)    // route already there
                  {
                    fprintf(stderr, "addRoute: src Jack midi route already exists.\n");
                    return;
                  }  
                }
                #ifdef ROUTE_DEBUG
                fprintf(stderr, "addRoute: src Jack dst Jack midi name: %s pushing source route\n", dst.device->name().toLatin1().constData());
                #endif
                routes->push_back(src);
              }  
              else
              {
                fprintf(stderr, "addRoute: source is Jack, but destination is not jack midi - type:%d\n", dst.device->deviceType());
                return;
              }
            }  
            else
            {
              fprintf(stderr, "addRoute: source is Jack, but destination is not track or midi - type:%d \n", dst.type);
              return;
            }
      }
      else if (dst.type == Route::JACK_ROUTE) 
      {
            if (src.type == Route::TRACK_ROUTE) 
            {
              if (src.track->type() != Track::AUDIO_OUTPUT) 
              {
                    fprintf(stderr, "addRoute: destination is jack, source is track but not audio output\n");
                    return;
              }
              if (src.channel < 0) 
              {
                    fprintf(stderr, "addRoute: destination is jack, source:%s is track but invalid channel:%d\n", src.track->name().toLatin1().constData(), src.channel);
                    return;
              }
              
              RouteList* outRoutes = src.track->outRoutes();
              dst.channel = src.channel;
              
              for (ciRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
              {
                    if (*i == dst)    // route already there
                    {
                          #ifdef ROUTE_DEBUG
                          fprintf(stderr, "addRoute: dst track route already exists.\n");
                          #endif
                          return;
                    }      
              }
              #ifdef ROUTE_DEBUG
              fprintf(stderr, "addRoute: dst Jack src track name: %s pushing destination route\n", src.track->name().toLatin1().constData());
              #endif
              outRoutes->push_back(dst);
            }
            else
            if (src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(src.device->deviceType() == MidiDevice::JACK_MIDI)
              {
                dst.channel = src.channel;
                RouteList* routes = src.device->outRoutes();
                for (ciRoute i = routes->begin(); i != routes->end(); ++i) 
                {
                  if (*i == dst)    // route already there
                  {
                    fprintf(stderr, "addRoute: dst Jack midi route already exists.\n");
                    return;
                  }  
                }
                #ifdef ROUTE_DEBUG
                fprintf(stderr, "addRoute: dst Jack src Jack midi name: %s pushing destination route\n", src.device->name().toLatin1().constData());
                #endif
                routes->push_back(dst);
              }
              else  
              {
                fprintf(stderr, "addRoute: destination is Jack, but source is not jack midi - type:%d\n", src.device->deviceType());
                return;
              }
            }
            else
            {
              fprintf(stderr, "addRoute: destination is Jack, but source is not track or midi - type:%d \n", src.type);
              return;
            }
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  
      {
            if(dst.type != Route::TRACK_ROUTE)
            {
              fprintf(stderr, "addRoute: source is midi port:%d, but destination is not track\n", src.midiPort);
              return;
            }
            
            MidiPort *mp = &MusEGlobal::midiPorts[src.midiPort];
            
            // Do not allow synth ports to connect to any track. It may be useful in some cases, 
            //  may be desired later, but for now it's just a routing hassle.  p4.0.35 
            if(mp->device() && mp->device()->isSynti())
              return;
            
            if(dst.channel < 1 || dst.channel >= (1 << MIDI_CHANNELS))
            {
              fprintf(stderr, "addRoute: source is midi port:%d, but destination channel mask:%d out of range\n", src.midiPort, dst.channel);
              return;
            }
                        
            src.channel = dst.channel;
            RouteList* outRoutes = mp->outRoutes();
            iRoute ir = outRoutes->begin();                                      
            for ( ; ir != outRoutes->end(); ++ir) 
            {
              if (ir->type == Route::TRACK_ROUTE && ir->track == dst.track)     // Does a route to the track exist?
              {
                ir->channel |= dst.channel;    // Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            #ifdef ROUTE_DEBUG
            fprintf(stderr, "addRoute: src midi port:%d dst track name:%s pushing dst and src routes\n", src.midiPort, dst.track->name().toLatin1().constData());
            #endif
            
            if(ir == outRoutes->end())    // Only if route not found, add the route, with the requested channel bits as mask to start with. 
              outRoutes->push_back(dst);
              
            RouteList* inRoutes = dst.track->inRoutes();
            
            // Make sure only one single route, with a channel mask, can ever exist.
            ir = inRoutes->begin();
            for ( ; ir != inRoutes->end(); ++ir)         
            {
              if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == src.midiPort)  // Does a route to the midi port exist?
              {
                ir->channel |= src.channel;    // Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            
            if(ir == inRoutes->end())    // Only if route not found, add the route, with the requested channel bits as mask to start with. 
              inRoutes->push_back(src);
      }
      else if(dst.type == Route::MIDI_PORT_ROUTE)  
      {
            if(src.type != Route::TRACK_ROUTE)
            {
              fprintf(stderr, "addRoute: destination is midi port:%d, but source is not track\n", dst.midiPort);
              return;
            }
            if(src.channel < 1 || src.channel >= (1 << MIDI_CHANNELS))
            {
              fprintf(stderr, "addRoute: destination is midi port:%d, but source channel mask:%d out of range\n", dst.midiPort, src.channel);
              return;
            }
            
            dst.channel = src.channel;
            RouteList* outRoutes = src.track->outRoutes();
      
            iRoute ir = outRoutes->begin();                                      
            for ( ; ir != outRoutes->end(); ++ir) 
            {
              if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == dst.midiPort)     // Does a route to the midi port exist?
              {
                ir->channel |= dst.channel;    // Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            
            if(ir == outRoutes->end())    // Only if route not found, add the route, with the requested channel bits as mask to start with. 
              outRoutes->push_back(dst);
            
            MidiPort *mp = &MusEGlobal::midiPorts[dst.midiPort];
            
            #ifdef ROUTE_DEBUG
            fprintf(stderr, "addRoute: src track:%s dst midi port:%d pushing dst and src routes\n", src.track->name().toLatin1().constData(), dst.midiPort);
            #endif
            RouteList* inRoutes = mp->inRoutes();
              
            // Make sure only one single route, with a channel mask, can ever exist.
            ir = inRoutes->begin();
            for ( ; ir != inRoutes->end(); ++ir)         
            {
              if (ir->type == Route::TRACK_ROUTE && ir->track == src.track)  // Does a route to the track exist?
              {
                ir->channel |= src.channel;    // Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            
            if(ir == inRoutes->end())    // Only if route not found, add the route, with the requested channel bits as mask to start with. 
              inRoutes->push_back(src);
      }
      else 
      {
        if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
        {
          fprintf(stderr, "addRoute: source or destination are not track routes\n");
          return;
        }
        
        RouteList* outRoutes = src.track->outRoutes();
        
        //
        // Must enforce to ensure channel and channels are valid if defaults of -1 passed.
        //
        if(src.track->type() == Track::AUDIO_SOFTSYNTH)
        {
          if(src.channel == -1)
            src.channel = 0;
          if(src.channels == -1)
            src.channels = src.track->channels();  
          dst.channel = src.channel;
          dst.channels = src.channels;
          dst.remoteChannel = src.remoteChannel;
        }
        
        for (ciRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
        {
              if (*i == dst)    // route already there
              // TODO: DELETETHIS all these comments wtf?
              //if (i->type == dst.type && i->channel == dst.channel)    
              {
                //if(i->type == Route::TRACK_ROUTE)
                {
                  //if(i->track == dst.track)
                  {
                    //if(i->channels == dst.channels)
                    {
                      //#ifdef ROUTE_DEBUG
                      fprintf(stderr, "addRoute: src track route already exists.\n");
                      //#endif
                      return;
                    }
                    //else
                    //{
                    
                    //}
                  }
                }
              }      
        }
        outRoutes->push_back(dst);
        RouteList* inRoutes;
        
        #ifdef ROUTE_DEBUG
        fprintf(stderr, "addRoute: src track ch:%d chs:%d remch:%d  dst track ch:%d chs:%d remch:%d name: %s pushing dest and source routes\n", 
          src.channel, src.channels, src.remoteChannel, dst.channel, dst.channels, dst.remoteChannel, dst.track->name().toLatin1().constData());
        #endif
        inRoutes = dst.track->inRoutes();
        
        // make sure AUDIO_AUX is processed last
        if (src.track->type() == Track::AUDIO_AUX)       // REMOVE Tim. This special aux code may not be useful or needed now.
              inRoutes->push_back(src);                  // so DELETETHIS?
        else
              inRoutes->insert(inRoutes->begin(), src);
        
        // Is the source an Aux Track or else does it have Aux Tracks routed to it?
        // Update the destination track's aux ref count, and all tracks it is routed to.
        if(src.track->auxRefCount())
            src.track->updateAuxRoute( src.track->auxRefCount(), dst.track );
        else 
        if(src.track->type() == Track::AUDIO_AUX)
            src.track->updateAuxRoute( 1, dst.track );
      }
}

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(Route src, Route dst)
{
      if (src.type == Route::JACK_ROUTE) 
      {
            if(!dst.isValid())
            {
              printf("removeRoute: source is jack, invalid destination\n");
              return;
            }
            
            if (dst.type == Route::TRACK_ROUTE) 
            {
              if (dst.track->type() != Track::AUDIO_INPUT) 
              {
                    fprintf(stderr, "removeRoute: source is jack, destination is track but not audio input\n");
                    return;
              }
              RouteList* inRoutes = dst.track->inRoutes();
              iRoute i;
              for (i = inRoutes->begin(); i != inRoutes->end(); ++i) 
              {
                    if (*i == src) 
                    {
                          inRoutes->erase(i);
                          break;
                    }
              }
            }  
            else
            if (dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              RouteList* routes = dst.device->inRoutes();
              iRoute i;
              for (i = routes->begin(); i != routes->end(); ++i) 
              {
                    if (*i == src) 
                    {
                          routes->erase(i);
                          break;
                    }
              }
            }  
            else
            {
                  fprintf(stderr, "removeRoute: source is jack, destination unknown\n");
                  return;
            }
      }
      else if (dst.type == Route::JACK_ROUTE) 
      {
            if(!src.isValid())
            {
              printf("removeRoute: destination is jack, invalid source\n");
              return;
            }
            
            if (src.type == Route::TRACK_ROUTE) 
            {
              if (src.track->type() != Track::AUDIO_OUTPUT) 
              {
                    fprintf(stderr, "removeRoute: destination is jack, source is track but not audio output\n");
                    return;
              }
              RouteList* outRoutes = src.track->outRoutes();
              iRoute i;
              for (i = outRoutes->begin(); i != outRoutes->end(); ++i) 
              {
                    if (*i == dst) {
                          outRoutes->erase(i);
                          break;
                          }
              }
            }  
            else
            if (src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              RouteList* routes = src.device->outRoutes();
              iRoute i;
              for (i = routes->begin(); i != routes->end(); ++i) 
              {
                    if (*i == dst) {
                          routes->erase(i);
                          break;
                          }
              }
            }  
            else
            {
                  fprintf(stderr, "removeRoute: destination is jack, source unknown\n");
                  return;
            }
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  
      {
        if(dst.type != Route::TRACK_ROUTE)
        {
          fprintf(stderr, "removeRoute: source is midi port:%d, but destination is not track\n", src.midiPort);
          return;
        }
        
        if(src.isValid())
        {
          MidiPort *mp = &MusEGlobal::midiPorts[src.midiPort];
          RouteList* outRoutes = mp->outRoutes();
          for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
          {
            if(i->type == Route::TRACK_ROUTE && i->track == dst.track)  // Is there a route to the track?
            {
              //printf("i->channel:%x dst.channel:%x\n", i->channel, dst.channel);   
              i->channel &= ~dst.channel;        // Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
              {
                //printf("erasing out route from midi port:%d\n", src.midiPort);   
                outRoutes->erase(i);
              }  
              
              break;  // For safety, keep looking and remove any more found.
                      // No, must break, else crash. There should only be one route anyway...
            }
          }
        }
        else
          printf("removeRoute: source is midi port:%d but invalid\n", src.midiPort); 
        
        if(dst.isValid())
        {
          RouteList* inRoutes = dst.track->inRoutes();
          for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
          {
            if (i->type == Route::MIDI_PORT_ROUTE && i->midiPort == src.midiPort)  // Is there a route to the midi port?
            {
              i->channel &= ~src.channel;        // Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                inRoutes->erase(i);
              
              break;  // For safety, keep looking and remove any more found.
                      // No, must break, else crash. There should only be one route anyway...
            }
          }
        }
        else
          printf("removeRoute: source is midi port:%d but destination track invalid\n", src.midiPort);
      }      
      else if(dst.type == Route::MIDI_PORT_ROUTE)  
      {
        if(src.type != Route::TRACK_ROUTE)
        {
          fprintf(stderr, "removeRoute: destination is midi port:%d, but source is not track\n", dst.midiPort);
          return;
        }
        
        if(src.isValid())
        {
          RouteList* outRoutes = src.track->outRoutes();
          for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
          {
            if (i->type == Route::MIDI_PORT_ROUTE && i->midiPort == dst.midiPort)  // Is there a route to the midi port?
            {
              i->channel &= ~dst.channel;        // Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                  outRoutes->erase(i);
                  
              break;  // For safety, keep looking and remove any more found.
                      // No, must break, else crash. There should only be one route anyway...
            }
          }
        }  
        else
          printf("removeRoute: destination is midi port:%d but source track is invalid\n", dst.midiPort);
        
        if(dst.isValid())
        {
          MidiPort *mp = &MusEGlobal::midiPorts[src.midiPort];
          RouteList* inRoutes = mp->inRoutes();
          for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
          {
            if (i->type == Route::TRACK_ROUTE && i->track == src.track)  // Is there a route to the track?
            {
              i->channel &= ~src.channel;        // Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                inRoutes->erase(i);
                  
              break;  // For safety, keep looking and remove any more found.
                      // No, must break, else crash. There should only be one route anyway...
            }
          }
        }  
        else
          printf("removeRoute: destination is midi port:%d but invalid\n", dst.midiPort);
      }
      else 
      {
            if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  
            {
              fprintf(stderr, "removeRoute: source and destination are not tracks\n");
              return;
            }
            
            // Is the source an Aux Track or else does it have Aux Tracks routed to it?
            // Update the destination track's aux ref count, and all tracks it is routed to.
            if(src.isValid() && dst.isValid())
            {
              if(src.track->auxRefCount())
                src.track->updateAuxRoute( -src.track->auxRefCount(), dst.track );
              else
              if(src.track->type() == Track::AUDIO_AUX)
                src.track->updateAuxRoute( -1, dst.track );
            }
            
            if(src.isValid())
            {
              RouteList* outRoutes = src.track->outRoutes();
              for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
              {
                    if (*i == dst) {
                          outRoutes->erase(i);
                          break;
                          }
              }
            }  
            else
              printf("removeRoute: source is track but invalid\n");
            
            if(dst.isValid())
            {
              RouteList* inRoutes;
              
              inRoutes = dst.track->inRoutes();
              for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
              {
                    if (*i == src) {
                          inRoutes->erase(i);
                          break;
                          }
              }
            }  
            else
              printf("removeRoute: source is track but destination invalid\n");
      }
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
        printf("removeAllRoutes: source is not midi device\n");
    }  
      
    if(dst.isValid())  
    {
      if(dst.type == Route::MIDI_DEVICE_ROUTE)
        dst.device->inRoutes()->clear();
      else
        printf("removeAllRoutes: dest is not midi device\n");
    }  
}

//---------------------------------------------------------
//   track2name
//    create string name representation for audio node
//---------------------------------------------------------

static QString track2name(const Track* n)
      {
      if (n == 0)
            return QWidget::tr("None");
      return n->name();
      }

//---------------------------------------------------------
//   name
//    create string name representation for audio node
//---------------------------------------------------------

QString Route::name() const
{
      if(type == MIDI_DEVICE_ROUTE) 
      {
        if(device)
        {
          // DELETETHIS 6
          // For unified jack in/out devices, the actual port names are now different from device name.
          // Like this:   device: "MyJackDevice1" ->  inport: "MyJackDevice1_in"  outport: "MyJackDevice1_out"
          /*  
          if(device->deviceType() == MidiDevice::JACK_MIDI)
            return MusEGlobal::audioDevice->portName(device->clientPort());
          else
          */
          
          return device->name();
        }
        return QWidget::tr("None");
      }
      else
      if(type == JACK_ROUTE) 
      {
        if (!MusEGlobal::checkAudioDevice()) return "";
        return MusEGlobal::audioDevice->portName(jackPort);
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
//   name2route
//---------------------------------------------------------

Route name2route(const QString& rn, bool /*dst*/, int rtype)
{
  int channel = -1;
  QString s(rn);
  // Support old route style in med files. Obsolete.
  if (rn[0].isNumber() && rn[1]==':') 
  {
    channel = rn[0].toAscii() - int('1');
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
      }
      else
      //if((rtype == Route::JACK_MIDI_ROUTE) || (rtype == Route::ALSA_MIDI_ROUTE))
      // TODO Distinguish the device types
      if(rtype == Route::MIDI_DEVICE_ROUTE)
      {  
        for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
        {
          if((*i)->name() == s)
            return Route(*i, channel);
          
          /* DELETETHIS 16?
          MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(*i);
          if(jmd)
          {     
            if(jmd->name() == s)
            //if (jmd->name() == rn)
                return Route(jmd);
          }      
          MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(*i);
          if(amd)
          {     
            // TODO
            if(amd->name() == s)
            //if (amd->name() == rn)
                return Route(amd);
          }      
          */
        }
      }
      else
      if(rtype == Route::JACK_ROUTE)
      {  
        if(MusEGlobal::checkAudioDevice())
        {
          void* p = MusEGlobal::audioDevice->findPort(s.toLatin1().constData());
          if(p)
            return Route(p, channel);
        }      
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
      }
  }
  
  printf("  name2route: <%s> not found\n", rn.toLatin1().constData());
  return Route((Track*) 0, channel);
}

//---------------------------------------------------------
//   checkRoute
//    return true if route is valid
//---------------------------------------------------------

bool checkRoute(const QString& s, const QString& d)
      {
      Route src(s, false, -1);
      Route dst(d, true, -1);

      if (!(src.isValid() && dst.isValid()) || (src == dst))
            return false;
      if (src.type == Route::JACK_ROUTE) 
      {
            if (dst.type == Route::TRACK_ROUTE) 
            {
              if (dst.track->type() != Track::AUDIO_INPUT) {
                    return false;
                    }
              src.channel = dst.channel;
              RouteList* inRoutes = dst.track->inRoutes();
              for (ciRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
              {
                    if (*i == src) {   // route already there
                          return false;
                          }
              }
            }
            else
            if (dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              src.channel = -1;
              RouteList* routes = dst.device->inRoutes();
              for (ciRoute i = routes->begin(); i != routes->end(); ++i) 
              {
                    if (*i == src) {   // route already there
                          return false;
                          }
              }
            }
            else
              return false;
      }  
      else if (dst.type == Route::JACK_ROUTE) 
      {
            if (src.type == Route::TRACK_ROUTE) 
            {
              if (src.track->type() != Track::AUDIO_OUTPUT) {
                    return false;
                    }
              RouteList* outRoutes = src.track->outRoutes();
              dst.channel = src.channel;
              for (ciRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
              {
                    if (*i == dst) {   // route already there
                          return false;
                          }
              }
            }
            else
            if (src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              RouteList* routes = src.device->outRoutes();
              dst.channel = -1;
              for (ciRoute i = routes->begin(); i != routes->end(); ++i) 
              {
                    if (*i == dst) {   // route already there
                          return false;
                          }
              }
            }
            else
              return false;
      }  
      else if (src.type == Route::MIDI_PORT_ROUTE) 
      {
            RouteList* outRoutes = MusEGlobal::midiPorts[src.midiPort].outRoutes();
            for (ciRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            {
                  if (*i == dst) {   // route already there
                        return false;
                        }
            }
      }
      else 
      {
            RouteList* outRoutes = (src.type == Route::MIDI_DEVICE_ROUTE) ? src.device->outRoutes() : src.track->outRoutes();
            
            for (ciRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            {
                  if (*i == dst) {   // route already there
                        return false;
                        }
            }
      }
      return true;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Route::read(Xml& xml)
{
      QString s;
      int dtype = MidiDevice::ALSA_MIDI;
      int port = -1;                             
      unsigned char rtype = Route::TRACK_ROUTE;  
      
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
                        printf("Route::read(): attribute:%s\n", tag.toLatin1().constData());
                        #endif
                        if(tag == "type")
                          rtype = xml.s2().toInt();
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
                        if(tag == "mport") 
                        {
                          port = xml.s2().toInt();
                          rtype = Route::MIDI_PORT_ROUTE;
                        }
                        else  
                          printf("Route::read(): unknown attribute:%s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        #ifdef ROUTE_DEBUG
                        printf("Route::read(): tag end type:%d channel:%d name:%s\n", rtype, channel, s.toLatin1().constData());
                        #endif
                        if(rtype == MIDI_PORT_ROUTE)  
                        {
                          if(port >= 0 && port < MIDI_PORTS)
                          {
                            type = rtype;
                            midiPort = port;
                          }
                          else
                            printf("Route::read(): midi port <%d> out of range\n", port);
                        }
                        else
                        if(!s.isEmpty())
                        {
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
                              printf("Route::read(): track <%s> not found\n", s.toLatin1().constData());
                          }
                          else
                          if(rtype == JACK_ROUTE) 
                          {
                            void* jport = 0;
                            if (MusEGlobal::audioDevice) // fix crash if jack is zombified at this point
                              jport=MusEGlobal::audioDevice->findPort(s.toLatin1().constData());
                            if(jport == 0)
                              printf("Route::read(): jack port <%s> not found\n", s.toLatin1().constData());
                            else
                            {
                              jackPort = jport;
                              type = rtype;
                            }
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
                                if(md->midiPort() == -1)
                                  break;
                                
                                device = md;
                                type = rtype;
                                break;
                              }
                            }
                            if(imd == MusEGlobal::midiDevices.end())
                              printf("Route::read(): midi device <%s> not found\n", s.toLatin1().constData());
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
      int chs         = -1;
      int remch       = -1;

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
                              droute.channel       = ch;
                              droute.channels      = chs;
                              droute.remoteChannel = remch;
                        }      
                        else
                              xml.unknown("readRoute");
                        break;
                  case Xml::Attribut:
                        #ifdef ROUTE_DEBUG
                        printf("Song::readRoute(): attribute:%s\n", tag.toLatin1().constData());
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
                          ch = xml.s2().toInt();               
                        else
                          printf("Song::readRoute(): unknown attribute:%s\n", tag.toLatin1().constData());
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
                            // Support pre- 1.1-RC2 midi-device-to-track routes. Obsolete. Replaced with midi port routes.
                            if(sroute.type == Route::MIDI_DEVICE_ROUTE && droute.type == Route::TRACK_ROUTE) 
                            {
                              if(sroute.device->midiPort() >= 0 && sroute.device->midiPort() < MIDI_PORTS
                                 && ch >= 0 && ch < MIDI_CHANNELS)         
                              {
                                sroute.midiPort = sroute.device->midiPort();
                                sroute.device = 0;
                                sroute.type = Route::MIDI_PORT_ROUTE;
                                
                                sroute.channel = 1 << ch;                  // Convert to new bit-wise channel mask.
                                droute.channel = sroute.channel;
                                
                                addRoute(sroute, droute);
                              }
                              else  
                                printf("  Warning - device:%s to track route, no device midi port or chan:%d out of range. Ignoring route!\n", 
                                       sroute.device->name().toLatin1().constData(), ch);
                            }
                            else if(sroute.type == Route::TRACK_ROUTE && droute.type == Route::MIDI_DEVICE_ROUTE) 
                            {
                              if(droute.device->midiPort() >= 0 && droute.device->midiPort() < MIDI_PORTS
                                 && ch >= 0 && ch < MIDI_CHANNELS)        
                              {
                                droute.midiPort = droute.device->midiPort();
                                droute.device = 0;
                                droute.type = Route::MIDI_PORT_ROUTE;
                                
                                droute.channel = 1 << ch;                  // Convert to new bit-wise channel mask.
                                sroute.channel = droute.channel;
                                
                                addRoute(sroute, droute);
                              }  
                              else  
                                printf("  Warning - track to device:%s route, no device midi port or chan:%d out of range. Ignoring route!\n", 
                                       droute.device->name().toLatin1().constData(), ch);
                            }
                            else
                            {
                              addRoute(sroute, droute);
                            }  
                          }
                          else
                            printf("  Warning - route invalid. Ignoring route!\n");
                          
                          return;
                        }
                  default:
                        break;
             }
      }
}

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteList::removeRoute(const Route& r)
      {
      for (iRoute i = begin(); i != end(); ++i) {
            if (r == *i) {
                  erase(i);
                  return;
                  }
            }
      printf("internal error: cannot remove Route\n");
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Route::dump() const
{
      if (type == TRACK_ROUTE)
      {
        if(track)
          printf("Route dump: track <%s> channel %d channels %d\n", track->name().toLatin1().constData(), channel, channels);
      }      
      else 
      if (type == JACK_ROUTE)
      {
        if(MusEGlobal::checkAudioDevice())
          printf("Route dump: jack audio port <%s> channel %d\n", MusEGlobal::audioDevice->portName(jackPort).toLatin1().constData(), channel);
      }
      else 
      if (type == MIDI_PORT_ROUTE) 
      {
        printf("Route dump: midi port <%d> channel mask %d\n", midiPort, channel);
      }
      else
      if (type == MIDI_DEVICE_ROUTE)
      {
        printf("Route dump: ");
        if(device)
        {
          if(device->deviceType() == MidiDevice::JACK_MIDI)
          {
            if(MusEGlobal::checkAudioDevice())
            {  
              printf("jack midi device <%s> ", device->name().toLatin1().constData());
              if(device->inClientPort())
                printf("input port <%s> ", 
                       MusEGlobal::audioDevice->portName(device->inClientPort()).toLatin1().constData());
              if(device->outClientPort())
                printf("output port <%s> ", 
                       MusEGlobal::audioDevice->portName(device->outClientPort()).toLatin1().constData());
            }           
          }
          else
          if(device->deviceType() == MidiDevice::ALSA_MIDI)
            printf("alsa midi device <%s> ", device->name().toLatin1().constData());
          else
          if(device->deviceType() == MidiDevice::SYNTH_MIDI)
            printf("synth midi device <%s> ", device->name().toLatin1().constData());
          else
            printf("is midi but unknown device type:%d, ", device->deviceType());
        }
        else
          printf("is midi but invalid device, ");
          
        printf("channel:%d\n", channel);
      }
      else
        printf("Route dump: unknown route type:%d\n", type);
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
            if(channel == a.channel)
            {
              if (type == JACK_ROUTE)
              {
                    //if (!MusEGlobal::checkAudioDevice()) return false; DELETETHIS? or keep?
                    //else return MusEGlobal::audioDevice->portName(jackPort) == MusEGlobal::audioDevice->portName(a.jackPort);
                    return jackPort == a.jackPort;    // Simplified.
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

/* yaaaay, depth search. just wrote an exam about that. but DELETETHIS, looks unused
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
