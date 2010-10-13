//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.cpp,v 1.18.2.3 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qwidget.h>

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

//#define ROUTE_MIDIPORT_NAME_PREFIX       "MusE MidiPort "
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

//Route::Route(AudioTrack* t, int ch)
Route::Route(Track* t, int ch, int chans)
//Route::Route(Track* t, int ch)
      {
      track    = t;
      midiPort = -1;
      channel  = ch;
      channels = chans;
      remoteChannel = -1;
      type     = TRACK_ROUTE;
      }

//Route::Route(MidiJackDevice* d)
Route::Route(MidiDevice* d, int ch)
{
      device   = d;  
      midiPort = -1;
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      /*
      //if(dynamic_cast<MidiJackDevice*>(d))
      if(d->deviceType() == MidiDevice::JACK_MIDI)
        type    = JACK_MIDI_ROUTE;
      else  
      //if(dynamic_cast<MidiAlsaDevice*>(d))
      if(d->deviceType() == MidiDevice::ALSA_MIDI)
        type    = ALSA_MIDI_ROUTE;
      */  
      type    = MIDI_DEVICE_ROUTE; 
}

Route::Route(int port, int ch)  // p3.3.49
{
      track    = 0;
      midiPort = port; 
      channel  = ch;
      channels = -1;
      remoteChannel = -1;
      type    = MIDI_PORT_ROUTE;     
}

//Route::Route(const QString& s, bool dst, int ch)
Route::Route(const QString& s, bool dst, int ch, int rtype)
    {
      //Route node(name2route(s, dst));
      Route node(name2route(s, dst, rtype));
      channel  = node.channel;
      if(channel == -1)
        channel = ch;
      //if(channels == -1)
      //  channels = chans;
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
      /*
      else
      if (type == JACK_MIDI_ROUTE)
            device = node.device;
      else
      if (type == ALSA_MIDI_ROUTE)
            device = node.device;
      */
      else
      if(type == MIDI_DEVICE_ROUTE)  
      {
        device = node.device;     
        midiPort = -1;
      }  
      else
      if(type == MIDI_PORT_ROUTE)     // p3.3.49
      {
        track = 0;
        midiPort = node.midiPort;     //
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
//         src.type, src.channel, src.name().latin1(),
//         dst.type, dst.channel, dst.name().latin1());
      if (src.type == Route::JACK_ROUTE) 
      {           
            //if (dst.type != TRACK_ROUTE) 
            //{
            //      fprintf(stderr, "addRoute: bad route 1\n");
                  // exit(-1);
            //      return;
            //}
            
            if (dst.type == Route::TRACK_ROUTE) 
            {
              if (dst.track->type() != Track::AUDIO_INPUT) 
              {
                  fprintf(stderr, "addRoute: source is jack, dest:%s is track but not audio input\n", dst.track->name().latin1());
                  //exit(-1);
                  return;
              }
              if (dst.channel < 0) 
              {
                  fprintf(stderr, "addRoute: source is jack, dest:%s is track but invalid channel:%d\n", dst.track->name().latin1(), dst.channel);
                  //exit(-1);
                  return;
              }
              
              //src.channel = src.dstChannel = dst.channel;
              src.channel = dst.channel;
              //src.channels = dst.channels = 1;
              RouteList* inRoutes = dst.track->inRoutes();
              for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
              {
                if (*i == src)    // route already there
                {
                  //#ifdef ROUTE_DEBUG
                  fprintf(stderr, "addRoute: src track route already exists.\n");
                  //#endif
                  return;
                }  
              }
              #ifdef ROUTE_DEBUG
              fprintf(stderr, "addRoute: src Jack dst track name: %s pushing source route\n", dst.track->name().latin1());
              #endif
              inRoutes->push_back(src);
            }  
            else
            //if (dst.type == Route::JACK_MIDI_ROUTE) 
            if (dst.type == Route::MIDI_DEVICE_ROUTE) 
            //if (dst.type == Route::MIDI_PORT_ROUTE)      // p3.3.49
            {
              //MidiDevice *md = midiPorts[dst.midiPort].device();
              //if(dst.device->deviceType() == MidiDevice::JACK_MIDI)
              //if(!md)
              //{
              //  fprintf(stderr, "addRoute: source is Jack, but no destination port device\n");
              //  return;
              //}
              
              if(dst.device->deviceType() == MidiDevice::JACK_MIDI)
              //if(md->deviceType() == MidiDevice::JACK_MIDI)
              {
                src.channel = dst.channel;
                //src.channel = -1;
                //src.channel = 0;
                //src.channel = src.dstChannel = dst.channel;
                //src.channels = dst.channels = 1;
                //dst.channel = -1;
                
                RouteList* routes = dst.device->inRoutes();
                for (iRoute i = routes->begin(); i != routes->end(); ++i) 
                {
                  if (*i == src)    // route already there
                  {
                    //#ifdef ROUTE_DEBUG
                    fprintf(stderr, "addRoute: src Jack midi route already exists.\n");
                    //#endif
                    return;
                  }  
                }
                #ifdef ROUTE_DEBUG
                fprintf(stderr, "addRoute: src Jack dst Jack midi name: %s pushing source route\n", dst.device->name().latin1());
                #endif
                routes->push_back(src);
              }  
              else
              {
                fprintf(stderr, "addRoute: source is Jack, but destination is not jack midi - type:%d\n", dst.device->deviceType());
                // exit(-1);
                return;
              }
            }  
            else
            {
              fprintf(stderr, "addRoute: source is Jack, but destination is not track or midi - type:%d \n", dst.type);
              // exit(-1);
              return;
            }
      }
      else if (dst.type == Route::JACK_ROUTE) 
      {
            //if (src.type != TRACK_ROUTE) 
            //{
            //      fprintf(stderr, "addRoute: bad route 3\n");
                  // exit(-1);
            //      return;
            //}
            
            if (src.type == Route::TRACK_ROUTE) 
            {
              if (src.track->type() != Track::AUDIO_OUTPUT) 
              {
                    fprintf(stderr, "addRoute: destination is jack, source is track but not audio output\n");
                    // exit(-1);
                    return;
              }
              if (src.channel < 0) 
              {
                    fprintf(stderr, "addRoute: destination is jack, source:%s is track but invalid channel:%d\n", src.track->name().latin1(), src.channel);
                    // exit(-1);
                    return;
              }
              
              RouteList* outRoutes = src.track->outRoutes();
              //dst.channel = dst.dstChannel = src.channel;
              dst.channel = src.channel;
              //dst.channels = src.channels = 1;
              
              for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
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
              fprintf(stderr, "addRoute: dst Jack src track name: %s pushing destination route\n", src.track->name().latin1());
              #endif
              outRoutes->push_back(dst);
            }
            else
            //if (src.type == Route::JACK_MIDI_ROUTE) 
            if (src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              if(src.device->deviceType() == MidiDevice::JACK_MIDI)
              {
                dst.channel = src.channel;
                //dst.channel = -1;
                //src.channel = -1;
                //dst.channel = dst.dstChannel = src.channel;
                //dst.channels = src.channels = 1;
                
                RouteList* routes = src.device->outRoutes();
                for (iRoute i = routes->begin(); i != routes->end(); ++i) 
                {
                  if (*i == dst)    // route already there
                  {
                    //#ifdef ROUTE_DEBUG
                    fprintf(stderr, "addRoute: dst Jack midi route already exists.\n");
                    //#endif
                    return;
                  }  
                }
                #ifdef ROUTE_DEBUG
                fprintf(stderr, "addRoute: dst Jack src Jack midi name: %s pushing destination route\n", src.device->name().latin1());
                #endif
                routes->push_back(dst);
              }
              else  
              {
                fprintf(stderr, "addRoute: destination is Jack, but source is not jack midi - type:%d\n", src.device->deviceType());
                // exit(-1);
                return;
              }
            }
            else
            {
              fprintf(stderr, "addRoute: destination is Jack, but source is not track or midi - type:%d \n", src.type);
              // exit(-1);
              return;
            }
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  // p3.3.49
      {
            if(dst.type != Route::TRACK_ROUTE)
            {
              fprintf(stderr, "addRoute: source is midi port:%d, but destination is not track\n", src.midiPort);
              return;
            }
            if(dst.channel < 1 || dst.channel >= (1 << MIDI_CHANNELS))
            {
              fprintf(stderr, "addRoute: source is midi port:%d, but destination channel mask:%d out of range\n", src.midiPort, dst.channel);
              return;
            }
            
            //MidiDevice *md = midiPorts[src.midiPort].device();
            //if(!md)
            //{
            //  fprintf(stderr, "addRoute: source is midi port, but no destination port device\n");
            //  return;
            //}
            
            MidiPort *mp = &midiPorts[src.midiPort];
            
            src.channel = dst.channel;
            RouteList* outRoutes = mp->outRoutes();
            //for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            iRoute ir = outRoutes->begin();                                      // p3.3.50
            for ( ; ir != outRoutes->end(); ++ir) 
            {
              //if (*i == dst)    // route already there
              if (ir->type == Route::TRACK_ROUTE && ir->track == dst.track)     // p3.3.50 Does a route to the track exist?
              {
                //#ifdef ROUTE_DEBUG
                //fprintf(stderr, "addRoute: src midi port:%d dst track:%s route already exists.\n", src.midiPort, dst.track->name().latin1());
                //#endif
                ir->channel |= dst.channel;    // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
                break;
                
                //return;
              }      
            }
            #ifdef ROUTE_DEBUG
            fprintf(stderr, "addRoute: src midi port:%d dst track name:%s pushing dst and src routes\n", src.midiPort, dst.track->name().latin1());
            #endif
            
            if(ir == outRoutes->end())    // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with. 
              outRoutes->push_back(dst);
              
            RouteList* inRoutes = dst.track->inRoutes();
            
            // p3.3.50 Make sure only one single route, with a channel mask, can ever exist.
            ir = inRoutes->begin();
            for ( ; ir != inRoutes->end(); ++ir)         
            {
              if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == src.midiPort)  // p3.3.50 Does a route to the midi port exist?
              {
                ir->channel |= src.channel;    // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            
            if(ir == inRoutes->end())    // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with. 
              inRoutes->push_back(src);
      }
      else if(dst.type == Route::MIDI_PORT_ROUTE)  // p3.3.49
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
            
            
            //MidiDevice *md = midiPorts[dst.midiPort].device();
            //if(!md)
            //{
            //  fprintf(stderr, "addRoute: dst is midi port, but no destination port device\n");
            //  return;
            //}
            
            dst.channel = src.channel;
            RouteList* outRoutes = src.track->outRoutes();
      
            //for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            iRoute ir = outRoutes->begin();                                      // p3.3.50
            for ( ; ir != outRoutes->end(); ++ir) 
            {
              //if (*i == dst)    // route already there
              if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == dst.midiPort)     // p3.3.50 Does a route to the midi port exist?
              {
                //#ifdef ROUTE_DEBUG
                //fprintf(stderr, "addRoute: dst midi port:%d src track:%s route already exists.\n", dst.midiPort, src.track->name().latin1());
                //#endif
                //return;
                
                ir->channel |= dst.channel;    // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            
            if(ir == outRoutes->end())    // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with. 
              outRoutes->push_back(dst);
            
            MidiPort *mp = &midiPorts[dst.midiPort];
            
            #ifdef ROUTE_DEBUG
            fprintf(stderr, "addRoute: src track:%s dst midi port:%d pushing dst and src routes\n", src.track->name().latin1(), dst.midiPort);
            #endif
            RouteList* inRoutes = mp->inRoutes();
              
            // p3.3.50 Make sure only one single route, with a channel mask, can ever exist.
            ir = inRoutes->begin();
            for ( ; ir != inRoutes->end(); ++ir)         
            {
              if (ir->type == Route::TRACK_ROUTE && ir->track == src.track)  // p3.3.50 Does a route to the track exist?
              {
                ir->channel |= src.channel;    // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
                break;
              }      
            }
            
            if(ir == inRoutes->end())    // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with. 
              inRoutes->push_back(src);
              //inRoutes->insert(inRoutes->begin(), src);
      }
      else 
      {
        if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  // p3.3.49
        {
          fprintf(stderr, "addRoute: source and destination are not track routes\n");
          return;
        }
        
        // Removed p3.3.49
        /*
        //if ((src.type == Route::JACK_MIDI_ROUTE) || (src.type == Route::ALSA_MIDI_ROUTE))
        if(src.type == Route::MIDI_DEVICE_ROUTE)
        {           
            //src.channel = src.dstChannel = dst.dstChannel = dst.channel;
            src.channel = dst.channel;
            //src.channels = dst.channels = 1;
            RouteList* outRoutes = src.device->outRoutes();
            #ifdef ROUTE_DEBUG
            fprintf(stderr, "addRoute: src name: %s looking for existing dest in out routes...\n", src.device->name().latin1());
            #endif
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            {
                  if (*i == dst)    // route already there
                  {
                        //#ifdef ROUTE_DEBUG
                        fprintf(stderr, "addRoute: src Jack or ALSA midi route already exists.\n");
                        //#endif
                        return;
                  }      
            }
            #ifdef ROUTE_DEBUG
            fprintf(stderr, "addRoute: src midi dst name: %s pushing destination and source routes\n", dst.track->name().latin1());
            #endif
            
            outRoutes->push_back(dst);
            RouteList* inRoutes = dst.track->inRoutes();
            inRoutes->push_back(src);
        }          
        else
        */
        
        {           
            ///if(dst.type == Route::MIDI_DEVICE_ROUTE)  // Removed p3.3.49
            //{
            ///  dst.channel = src.channel;
              //src.channel = src.dstChannel = dst.dstChannel = dst.channel;
              //src.channels = dst.channels = 1;
            //}
            //else
            //{
              //src.channel = src.dstChannel = dst.dstChannel = dst.channel;
              //src.channels = dst.channels = 1;
            //}
              
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
              //if(dst.type == Route::TRACK_ROUTE)      // p3.3.49 Removed
              //{
                //if(dst.channel == -1)
                //  dst.channel = 0;
                //if(dst.channels == -1)
                  // Yes, that's correct: dst channels = src track channels.
                //  dst.channels = src.track->channels();  
                dst.channel = src.channel;
                dst.channels = src.channels;
                dst.remoteChannel = src.remoteChannel;
              //}
            }
            //if(dst.type == Route::TRACK_ROUTE && dst.track->type() == Track::AUDIO_SOFTSYNTH)
            //{
            //  if(dst.channel == -1)
            //    dst.channel = 0;
            //  if(dst.channels == -1)
                // Yes, that's correct: dst channels = src track channels.
            //    dst.channels = src.track->channels();  
            //}
            
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            {
                  if (*i == dst)    // route already there
                  // TODO:
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
            
            // Removed p3.3.49
            /*
            //if ((dst.type == Route::JACK_MIDI_ROUTE) || (dst.type == Route::ALSA_MIDI_ROUTE))
            if(dst.type == Route::MIDI_DEVICE_ROUTE)
            {
              #ifdef ROUTE_DEBUG
              fprintf(stderr, "addRoute: src track dst midi name: %s pushing destination and source routes\n", dst.device->name().latin1());
              #endif
              inRoutes = dst.device->inRoutes();
            }  
            else  
            */
            
            {
              #ifdef ROUTE_DEBUG
              //fprintf(stderr, "addRoute: src track ch:%d chs:%d  dst track ch:%d chs:%d name: %s pushing destination and source routes\n", src.channel, src.channels, dst.channel, dst.channels, dst.track->name().latin1());
              fprintf(stderr, "addRoute: src track ch:%d chs:%d remch:%d  dst track ch:%d chs:%d remch:%d name: %s pushing dest and source routes\n", 
                src.channel, src.channels, src.remoteChannel, dst.channel, dst.channels, dst.remoteChannel, dst.track->name().latin1());
              //fprintf(stderr, "addRoute: src track ch:%d  dst track ch:%d name: %s pushing destination and source routes\n", src.channel, dst.channel, dst.track->name().latin1());
              #endif
              inRoutes = dst.track->inRoutes();
            }  
              
            
            //
            // make sure AUDIO_AUX is processed last
            //
            if (src.track->type() == Track::AUDIO_AUX)
                  inRoutes->push_back(src);
            else
                  inRoutes->insert(inRoutes->begin(), src);
        }          
      }
}

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(Route src, Route dst)
{
    //printf("removeRoute %d.%d:<%s> %d.%d:<%s>\n",
    //     src.type, src.channel, src.name().latin1(),
    //     dst.type, dst.channel, dst.name().latin1());
         
      if (src.type == Route::JACK_ROUTE) 
      {
            //if (dst.type != TRACK_ROUTE) 
            //{
            //      fprintf(stderr, "removeRoute: bad route 1\n");
                  // exit(-1);
            //      return;
            //}
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
                    // exit(-1);
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
            //if (dst.type == Route::JACK_MIDI_ROUTE) 
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
                  // exit(-1);
                  return;
            }
      }
      else if (dst.type == Route::JACK_ROUTE) 
      {
            //if (src.type != TRACK_ROUTE) 
            //{
            //      fprintf(stderr, "removeRoute: bad route 3\n");
                  // exit(-1);
            //      return;
            //}
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
                    // exit(-1);
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
            //if (src.type == Route::JACK_MIDI_ROUTE) 
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
                  // exit(-1);
                  return;
            }
      }
      else if(src.type == Route::MIDI_PORT_ROUTE)  // p3.3.49
      {
        if(dst.type != Route::TRACK_ROUTE)
        {
          fprintf(stderr, "removeRoute: source is midi port:%d, but destination is not track\n", src.midiPort);
          return;
        }
        
        if(src.isValid())
        {
          MidiPort *mp = &midiPorts[src.midiPort];
          RouteList* outRoutes = mp->outRoutes();
          for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
          {
            //if (*i == dst) 
            if (i->type == Route::TRACK_ROUTE && i->track == dst.track)  // p3.3.50 Is there a route to the track?
            {
              i->channel &= ~dst.channel;        // p3.3.50 Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                outRoutes->erase(i);
                
              break;                             // For safety, keep looking and remove any more found.
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
            //if (*i == src) 
            if (i->type == Route::MIDI_PORT_ROUTE && i->midiPort == src.midiPort)  // p3.3.50 Is there a route to the midi port?
            {
              i->channel &= ~src.channel;        // p3.3.50 Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                inRoutes->erase(i);
                
              break;                             // For safety, keep looking and remove any more found.
                                                 // No, must break, else crash. There should only be one route anyway...
            }
          }
        }
        else
          printf("removeRoute: source is midi port:%d but destination track invalid\n", src.midiPort);
      }      
      else if(dst.type == Route::MIDI_PORT_ROUTE)  // p3.3.49
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
            //if (*i == dst) 
            if (i->type == Route::MIDI_PORT_ROUTE && i->midiPort == dst.midiPort)  // p3.3.50 Is there a route to the midi port?
            {
              i->channel &= ~dst.channel;        // p3.3.50 Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                  outRoutes->erase(i);
                  
              break;                             // For safety, keep looking and remove any more found.
                                                 // No, must break, else crash. There should only be one route anyway...
            }
          }
        }  
        else
          printf("removeRoute: destination is midi port:%d but source track is invalid\n", dst.midiPort);
        
        if(dst.isValid())
        {
          MidiPort *mp = &midiPorts[src.midiPort];
          RouteList* inRoutes = mp->inRoutes();
          for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
          {
            //if (*i == src) 
            if (i->type == Route::TRACK_ROUTE && i->track == src.track)  // p3.3.50 Is there a route to the track?
            {
              i->channel &= ~src.channel;        // p3.3.50 Unset the desired channel bits.
              if(i->channel == 0)                // Only if there are no channel bits set, erase the route.
                inRoutes->erase(i);
                  
              break;                             // For safety, keep looking and remove any more found.
                                                 // No, must break, else crash. There should only be one route anyway...
            }
          }
        }  
        else
          printf("removeRoute: destination is midi port:%d but invalid\n", dst.midiPort);
      }
      else 
      {
            if(src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE)  // p3.3.49
            {
              fprintf(stderr, "removeRoute: source and destination are not tracks\n");
              return;
            }
            
            // Removed p3.3.49
            /*
            //if((src.type == Route::JACK_MIDI_ROUTE) || (src.type == Route::ALSA_MIDI_ROUTE))
            if(src.type == Route::MIDI_DEVICE_ROUTE)
            {
              if(src.isValid())
              {
                RouteList* outRoutes = src.device->outRoutes();
                for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
                {
                      if (*i == dst) {
                            outRoutes->erase(i);
                            break;
                            }
                }
              }
              else
                printf("removeRoute: source is midi but invalid\n");
              
              if(dst.isValid())
              {
                RouteList* inRoutes = dst.track->inRoutes();
                for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
                {
                      if (*i == src) {
                            inRoutes->erase(i);
                            break;
                            }
                }
              }
              else
                printf("removeRoute: source is midi but destination invalid\n");
            }
            else
            */
            
            {
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
                
                //if ((dst.type == Route::JACK_MIDI_ROUTE) || (dst.type == Route::ALSA_MIDI_ROUTE))
                // Removed p3.3.49
                /*
                if (dst.type == Route::MIDI_DEVICE_ROUTE)
                  inRoutes = dst.device->inRoutes();
                else  
                */
                
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
      // p3.3.38 Removed
      /*
      QString s;
      if ((type == TRACK_ROUTE) && (channel != -1)) {
//      if (channel != -1) {
            QString c;
            c.setNum(channel+1);
            s = c + ":";
            }
      */
      
      if(type == MIDI_DEVICE_ROUTE) 
      {
        if(device)
        {
          if(device->deviceType() == MidiDevice::JACK_MIDI)
            return audioDevice->portName(device->clientPort());
          else
          //if(device->deviceType() == MidiDevice::ALSA_MIDI)
            return device->name();
        }
        return QWidget::tr("None");
      }
      else
      if(type == JACK_ROUTE) 
      {
        if (!checkAudioDevice()) return "";
        //return s + audioDevice->portName(jackPort);
        return audioDevice->portName(jackPort);
      }
      else
      if(type == MIDI_PORT_ROUTE) // p3.3.49
      {
        return ROUTE_MIDIPORT_NAME_PREFIX + QString().setNum(midiPort);
      }
      else
        //return s + track2name(track);
        return track2name(track);
}

//---------------------------------------------------------
//   name2route
//---------------------------------------------------------

//Route name2route(const QString& rn, bool dst)
Route name2route(const QString& rn, bool /*dst*/, int rtype)
{
// printf("name2route %s\n", rn.latin1());
  int channel = -1;
  //int channel = 0;
  QString s(rn);
  // Support old route style in med files. Obsolete.
  if (rn[0].isNumber() && rn[1]==':') 
  {
    channel = rn[0].toAscii() - int('1');
    s = rn.mid(2);
  }
  
  if(rtype == -1)
  {  
    //if(dst) 
    //{
      if(checkAudioDevice())
      {
        void* p = audioDevice->findPort(s.latin1());
        if(p)
          return Route(p, channel);
      }
      
      TrackList* tl = song->tracks();
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
      
      for(iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i) 
      {
        if((*i)->name() == s)
            return Route(*i, channel);
      }
    
      // p3.3.49
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
    //if(dst) 
    //{
      if(rtype == Route::TRACK_ROUTE)
      {  
        TrackList* tl = song->tracks();
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
              //return Route(track, channel, 1);
              //return Route(track, channel, track->channels());
          }      
        }
      }
      else
      //if((rtype == Route::JACK_MIDI_ROUTE) || (rtype == Route::ALSA_MIDI_ROUTE))
      // TODO Distinguish the device types
      if(rtype == Route::MIDI_DEVICE_ROUTE)
      {  
        for(iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i) 
        {
          if((*i)->name() == s)
          //if (jmd->name() == rn)
            return Route(*i, channel);
          
          /*
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
        if(checkAudioDevice())
        {
          void* p = audioDevice->findPort(s.latin1());
          if(p)
            return Route(p, channel);
        }      
      }
      else
      if(rtype == Route::MIDI_PORT_ROUTE) // p3.3.49
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
  
  printf("  name2route: <%s> not found\n", rn.latin1());
  return Route((Track*) 0, channel);
  //return Route((Track*) 0, channel, 1);
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
            //if (dst.type != TRACK_ROUTE) {
            //      return false;
            //      }
            
            if (dst.type == Route::TRACK_ROUTE) 
            {
              if (dst.track->type() != Track::AUDIO_INPUT) {
                    return false;
                    }
              src.channel = dst.channel;
              RouteList* inRoutes = dst.track->inRoutes();
              for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i) 
              {
                    if (*i == src) {   // route already there
                          return false;
                          }
              }
            }
            else
            //if (dst.type == Route::JACK_MIDI_ROUTE) 
            if (dst.type == Route::MIDI_DEVICE_ROUTE) 
            {
              //src.channel = dst.channel;
              src.channel = -1;
              //dst.channel = -1;
              RouteList* routes = dst.device->inRoutes();
              for (iRoute i = routes->begin(); i != routes->end(); ++i) 
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
            //if (src.type != TRACK_ROUTE) {
            //      return false;
            //      }
            
            if (src.type == Route::TRACK_ROUTE) 
            {
              if (src.track->type() != Track::AUDIO_OUTPUT) {
                    return false;
                    }
              RouteList* outRoutes = src.track->outRoutes();
              dst.channel = src.channel;
              for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
              {
                    if (*i == dst) {   // route already there
                          return false;
                          }
              }
            }
            else
            //if (src.type == Route::JACK_MIDI_ROUTE) 
            if (src.type == Route::MIDI_DEVICE_ROUTE) 
            {
              RouteList* routes = src.device->outRoutes();
              //dst.channel = src.channel;
              dst.channel = -1;
              //src.channel = -1;
              for (iRoute i = routes->begin(); i != routes->end(); ++i) 
              {
                    if (*i == dst) {   // route already there
                          return false;
                          }
              }
            }
            else
              return false;
      }  
      else if (src.type == Route::MIDI_PORT_ROUTE) // p3.3.49
      {
            RouteList* outRoutes = midiPorts[src.midiPort].outRoutes();
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
            {
                  if (*i == dst) {   // route already there
                        return false;
                        }
            }
      }
      //else if (dst.type == Route::MIDI_PORT_ROUTE) // p3.3.49
      //{
      //}
      else 
      {
            //RouteList* outRoutes = ((src.type == Route::JACK_MIDI_ROUTE) || (src.type == Route::ALSA_MIDI_ROUTE)) ? 
            //                       src.device->outRoutes() : src.track->outRoutes();
            RouteList* outRoutes = (src.type == Route::MIDI_DEVICE_ROUTE) ? src.device->outRoutes() : src.track->outRoutes();
            
            for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i) 
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
      int port = -1;                             // p3.3.49
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
                  //case Xml::TagStart:
                  //        xml.unknown("Route");
                  //      break;
                  case Xml::Attribut:
                        #ifdef ROUTE_DEBUG
                        printf("Route::read(): attribute:%s\n", tag.latin1());
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
                        if(tag == "mport") // p3.3.49
                        {
                          port = xml.s2().toInt();
                          rtype = Route::MIDI_PORT_ROUTE;
                        }
                        else  
                          printf("Route::read(): unknown attribute:%s\n", tag.latin1());
                        break;
                  case Xml::TagEnd:
                        #ifdef ROUTE_DEBUG
                        printf("Route::read(): tag end type:%d channel:%d name:%s\n", rtype, channel, s.latin1());
                        #endif
                        if(rtype == MIDI_PORT_ROUTE)  // p3.3.49
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
                            TrackList* tl = song->tracks();
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
                              printf("Route::read(): track <%s> not found\n", s.latin1());
                          }
                          else
                          if(rtype == JACK_ROUTE) 
                          {
                            void* jport = audioDevice->findPort(s);
                            if(jport == 0)
                              printf("Route::read(): jack port <%s> not found\n", s.latin1());
                            else
                            {
                              jackPort = jport;
                              type = rtype;
                            }
                          }
                          else
                          if(rtype == MIDI_DEVICE_ROUTE)
                          {
                            iMidiDevice imd = midiDevices.begin();
                            for( ; imd != midiDevices.end(); ++imd) 
                            {
                              MidiDevice* md = *imd;
                              if(md->name() == s && md->deviceType() == dtype) 
                              {
                                // p3.3.45
                                // We found a device, but if it is not in use by the song (port is -1), ignore it. 
                                // This prevents loading and propagation of bogus routes in the med file.
                                if(md->midiPort() == -1)
                                  break;
                                
                                device = md;
                                type = rtype;
                                break;
                              }
                            }
                            if(imd == midiDevices.end())
                              printf("Route::read(): midi device <%s> not found\n", s.latin1());
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
                        // p3.3.38 2010/02/03 Support old routes in med files. Now obsolete!
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
                        printf("Song::readRoute(): attribute:%s\n", tag.latin1());
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
                        if(tag == "channelMask")           // p3.3.50 New channel mask for midi port-track routes.
                          ch = xml.s2().toInt();               
                        else
                          printf("Song::readRoute(): unknown attribute:%s\n", tag.latin1());
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
                            // p3.3.49 Support pre- 1.1-RC2 midi-device-to-track routes. Obsolete. Replaced with midi port routes.
                            if(sroute.type == Route::MIDI_DEVICE_ROUTE && droute.type == Route::TRACK_ROUTE) 
                            {
                              if(sroute.device->midiPort() >= 0 && sroute.device->midiPort() < MIDI_PORTS
                                 && ch >= 0 && ch < MIDI_CHANNELS)         // p3.3.50
                              {
                                sroute.midiPort = sroute.device->midiPort();
                                sroute.device = 0;
                                sroute.type = Route::MIDI_PORT_ROUTE;
                                
                                sroute.channel = 1 << ch;                  // p3.3.50  Convert to new bit-wise channel mask.
                                droute.channel = sroute.channel;
                                
                                addRoute(sroute, droute);
                              }
                              else  
                                printf("  Warning - device:%s to track route, no device midi port or chan:%d out of range. Ignoring route!\n", 
                                       sroute.device->name().latin1(), ch);
                            }
                            else if(sroute.type == Route::TRACK_ROUTE && droute.type == Route::MIDI_DEVICE_ROUTE) 
                            {
                              if(droute.device->midiPort() >= 0 && droute.device->midiPort() < MIDI_PORTS
                                 && ch >= 0 && ch < MIDI_CHANNELS)        // p3.3.50
                              {
                                droute.midiPort = droute.device->midiPort();
                                droute.device = 0;
                                droute.type = Route::MIDI_PORT_ROUTE;
                                
                                droute.channel = 1 << ch;                  // p3.3.50  Convert to new bit-wise channel mask.
                                sroute.channel = droute.channel;
                                
                                addRoute(sroute, droute);
                              }  
                              else  
                                printf("  Warning - track to device:%s route, no device midi port or chan:%d out of range. Ignoring route!\n", 
                                       droute.device->name().latin1(), ch);
                            }
                            else
                            {
                              //printf("adding new route...\n");
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
      //printf("RouteList::removeRoute:\n");
      //r.dump();  
      //printf("Searching routes:\n");
      
      for (iRoute i = begin(); i != end(); ++i) {
            //i->dump();  
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
          printf("Route dump: track <%s> channel %d channels %d\n", track->name().latin1(), channel, channels);
          //printf("Route dump: track <%s> channel %d\n", track->name().latin1(), channel);
        //else  
        //  printf("Route dump: invalid track, channel %d\n", channel);
      }      
      else 
      if (type == JACK_ROUTE)
      {
        if(checkAudioDevice())
          printf("Route dump: jack audio port <%s> channel %d\n", audioDevice->portName(jackPort).latin1(), channel);
      }
      else 
      if (type == MIDI_PORT_ROUTE) // p3.3.49
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
            if(checkAudioDevice())
              printf("jack midi port device <%s> ", audioDevice->portName(device->clientPort()).latin1());
          }
          else
          if(device->deviceType() == MidiDevice::ALSA_MIDI)
            printf("alsa midi device <%s> ", device->name().latin1());
          else
          if(device->deviceType() == MidiDevice::SYNTH_MIDI)
            printf("synth midi device <%s> ", device->name().latin1());
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
      //if (type == MIDI_PORT_ROUTE) // p3.3.50
      //{
        // Use new channel mask. True if all the bits in a.channel are contained in this route's channel.
        // Hmm, not commutative... Two such routes are equal if _____ what? ...   Code-specific for now.
      //  return midiPort == a.midiPort && (channel & a.channel) == a.channel;  
      //}
      //else        
      
      if ((type == a.type) && (channel == a.channel)) 
      //if (type == a.type) 
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
                    if (!checkAudioDevice()) return false;
                    return audioDevice->portName(jackPort) == audioDevice->portName(a.jackPort);
              }
              else 
              if (type == MIDI_PORT_ROUTE) // p3.3.49
              {
                return midiPort == a.midiPort;
              }
              else 
              if (type == MIDI_DEVICE_ROUTE)
              {
                if(device && a.device && device->deviceType() == a.device->deviceType())
                {
                  if(device->deviceType() == MidiDevice::JACK_MIDI)
                  {
                    if (!checkAudioDevice()) return false;
                    return audioDevice->portName(device->clientPort()) == audioDevice->portName(a.device->clientPort());
                  }
                  else
                  if(device->deviceType() == MidiDevice::ALSA_MIDI)
                    // TODO: OK ?? 
                    return device->clientPort() == a.device->clientPort() && (channel == a.channel);
                  else
                  if(device->deviceType() == MidiDevice::SYNTH_MIDI)
                    return device->name() == a.device->name();
                }    
              }
            }    
      }
      return false;
}

