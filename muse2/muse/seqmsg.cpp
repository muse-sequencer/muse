//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: seqmsg.cpp,v 1.32.2.17 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include "song.h"
#include "midiport.h"
#include "minstrument.h"
#include "app.h"
#include "amixer.h"
#include "tempo.h"
///#include "sig.h"
#include "al/sig.h"
#include "audio.h"
#include "mididev.h"
#include "audiodev.h"
#include "alsamidi.h"
#include "audio.h"
#include "arranger.h"
#include "plugin.h"
#include "driver/jackmidi.h"

//---------------------------------------------------------
//   sendMsg
//---------------------------------------------------------

void Audio::sendMsg(AudioMsg* m)
      {
      static int sno = 0;

      if (_running) {
            m->serialNo = sno++;
            //DEBUG:
            msg = m;
            // wait for next audio "process" call to finish operation
            int no = -1;
            int rv = read(fromThreadFdr, &no, sizeof(int));
            if (rv != sizeof(int))
                  perror("Audio: read pipe failed");
            else if (no != (sno-1)) {
                  fprintf(stderr, "audio: bad serial number, read %d expected %d\n",
                     no, sno-1);
                  }
            }
      else {
            // if audio is not running (during initialization)
            // process commands immediatly
            processMsg(m);
            }
      }

//---------------------------------------------------------
//   sendMessage
//    send request from gui to sequencer
//    wait until request is processed
//---------------------------------------------------------

bool Audio::sendMessage(AudioMsg* m, bool doUndo)
      {
      if (doUndo)
           song->startUndo();
      sendMsg(m);
      if (doUndo)
            song->endUndo(0);       // song->endMsgCmd();
      return false;
      }

//---------------------------------------------------------
//   msgRemoveRoute
//---------------------------------------------------------

void Audio::msgRemoveRoute(Route src, Route dst)
{
      msgRemoveRoute1(src, dst);
      //if (!checkAudioDevice()) return;
      if (src.type == Route::JACK_ROUTE)
      {
          if (!checkAudioDevice()) return;
          
          //if(dst.type == Route::JACK_MIDI_ROUTE)  
          if(dst.type == Route::MIDI_DEVICE_ROUTE)  
          {
            //MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(dst.device);
            //if(jmd)
            if(dst.device)
            {
              if(dst.device->deviceType() == MidiDevice::JACK_MIDI)
                //audioDevice->disconnect(src.jackPort, dst.device->clientPort());                                           
                audioDevice->disconnect(src.jackPort, dst.device->inClientPort());     // p3.3.55
              //else
              //{
                // TODO...
                //MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(dst.device);
                //if(amd)
              //}  
            }
          }
          else  
            audioDevice->disconnect(src.jackPort, ((AudioInput*)dst.track)->jackPort(dst.channel));
      }
      else if (dst.type == Route::JACK_ROUTE)
      {
          if (!checkAudioDevice()) return;
          
          //if(src.type == Route::JACK_MIDI_ROUTE)  
          if(src.type == Route::MIDI_DEVICE_ROUTE)  
          {
            //MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(src.device);
            //if(jmd)
            if(src.device)
            {
              if(src.device->deviceType() == MidiDevice::JACK_MIDI)
                //audioDevice->disconnect(src.device->clientPort(), dst.jackPort);
                audioDevice->disconnect(src.device->outClientPort(), dst.jackPort);     // p3.3.55
              //else
              //{
                // TODO...
                //MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(src.device);
                //if(amd)
              //}
            }  
          }
          else  
            audioDevice->disconnect(((AudioOutput*)src.track)->jackPort(src.channel), dst.jackPort);
      }
}

//---------------------------------------------------------
//   msgRemoveRoute1
//---------------------------------------------------------

void Audio::msgRemoveRoute1(Route src, Route dst)
      {
      AudioMsg msg;
      msg.id     = AUDIO_ROUTEREMOVE;
      msg.sroute = src;
      msg.droute = dst;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgRemoveRoutes
//---------------------------------------------------------

// p3.3.55
void Audio::msgRemoveRoutes(Route src, Route dst)
{
      msgRemoveRoutes1(src, dst);
      
      // TODO
      /*
      //if (!checkAudioDevice()) return;
      if (src.type == Route::JACK_ROUTE)
      {
          if (!checkAudioDevice()) return;
          
          //if(dst.type == Route::JACK_MIDI_ROUTE)  
          if(dst.type == Route::MIDI_DEVICE_ROUTE)  
          {
            //MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(dst.device);
            //if(jmd)
            if(dst.device)
            {
              if(dst.device->deviceType() == MidiDevice::JACK_MIDI)
                //audioDevice->disconnect(src.jackPort, dst.device->clientPort());
                audioDevice->disconnect(src.jackPort, dst.device->inClientPort());     
              //else
              //{
                // TODO...
                //MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(dst.device);
                //if(amd)
              //}  
            }
          }
          else  
            audioDevice->disconnect(src.jackPort, ((AudioInput*)dst.track)->jackPort(dst.channel));
      }
      else if (dst.type == Route::JACK_ROUTE)
      {
          if (!checkAudioDevice()) return;
          
          //if(src.type == Route::JACK_MIDI_ROUTE)  
          if(src.type == Route::MIDI_DEVICE_ROUTE)  
          {
            //MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(src.device);
            //if(jmd)
            if(src.device)
            {
              if(src.device->deviceType() == MidiDevice::JACK_MIDI)
                //audioDevice->disconnect(src.device->clientPort(), dst.jackPort);
                audioDevice->disconnect(src.device->outClientPort(), dst.jackPort);    
              //else
              //{
                // TODO...
                //MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(src.device);
                //if(amd)
              //}
            }  
          }
          else  
            audioDevice->disconnect(((AudioOutput*)src.track)->jackPort(src.channel), dst.jackPort);
      }
      
      */  
}

//---------------------------------------------------------
//   msgRemoveRoutes1
//---------------------------------------------------------

// p3.3.55
void Audio::msgRemoveRoutes1(Route src, Route dst)
      {
      AudioMsg msg;
      msg.id     = AUDIO_REMOVEROUTES;
      msg.sroute = src;
      msg.droute = dst;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgAddRoute
//---------------------------------------------------------

void Audio::msgAddRoute(Route src, Route dst)
      {
      if (src.type == Route::JACK_ROUTE) 
      {
            if (!checkAudioDevice()) return;
            if (isRunning())
            {
                //if(dst.type == Route::JACK_MIDI_ROUTE)  
                if(dst.type == Route::MIDI_DEVICE_ROUTE)  
                {
                  //MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(dst.device);
                  //if(jmd)
                  if(dst.device)
                  {
                    if(dst.device->deviceType() == MidiDevice::JACK_MIDI)  
                      //audioDevice->connect(src.jackPort, dst.device->clientPort());
                      audioDevice->connect(src.jackPort, dst.device->inClientPort());    // p3.3.55
                    //else
                    //{
                      // TODO...
                      //MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(dst.device);
                      //if(amd)
                    //}
                  }  
                }
                else  
                  audioDevice->connect(src.jackPort, ((AudioInput*)dst.track)->jackPort(dst.channel));
            }      
      }
      else if (dst.type == Route::JACK_ROUTE) 
      {
            if (!checkAudioDevice()) return;
            if (audio->isRunning())
            {
                //if(src.type == Route::JACK_MIDI_ROUTE)  
                if(src.type == Route::MIDI_DEVICE_ROUTE)  
                {
                  //MidiJackDevice* jmd = dynamic_cast<MidiJackDevice*>(src.device);
                  //if(jmd)
                  if(src.device)
                  {
                    if(src.device->deviceType() == MidiDevice::JACK_MIDI)  
                      //audioDevice->connect(src.device->clientPort(), dst.jackPort);
                      audioDevice->connect(src.device->outClientPort(), dst.jackPort);      // p3.3.55
                    //else
                    //{
                      // TODO...
                      //MidiAlsaDevice* amd = dynamic_cast<MidiAlsaDevice*>(src.device);
                      //if(amd)
                    //}
                  }  
                }
                else  
                  audioDevice->connect(((AudioOutput*)src.track)->jackPort(dst.channel), dst.jackPort);
            }      
      }
      msgAddRoute1(src, dst);
      }

//---------------------------------------------------------
//   msgAddRoute1
//---------------------------------------------------------

void Audio::msgAddRoute1(Route src, Route dst)
      {
      AudioMsg msg;
      msg.id = AUDIO_ROUTEADD;
      msg.sroute = src;
      msg.droute = dst;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgAddPlugin
//---------------------------------------------------------

void Audio::msgAddPlugin(AudioTrack* node, int idx, PluginI* plugin)
      {
      AudioMsg msg;
      msg.id     = AUDIO_ADDPLUGIN;
      msg.snode  = node;
      msg.ival   = idx;
      msg.plugin = plugin;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetRecord
//---------------------------------------------------------

void Audio::msgSetRecord(AudioTrack* node, bool val)
      {
      AudioMsg msg;
      msg.id     = AUDIO_RECORD;
      msg.snode  = node;
      msg.ival   = int(val);
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetVolume
//---------------------------------------------------------

void Audio::msgSetVolume(AudioTrack* src, double val)
      {
      AudioMsg msg;
      msg.id    = AUDIO_VOL;
      msg.snode    = src;
      msg.dval  = val;
      sendMsg(&msg);
      //muse->arranger->controllerChanged(src);
      }

//---------------------------------------------------------
//   msgSetPan
//---------------------------------------------------------

void Audio::msgSetPan(AudioTrack* node, double val)
      {
      AudioMsg msg;
      msg.id    = AUDIO_PAN;
      msg.snode = node;
      msg.dval  = val;
      sendMsg(&msg);
      //muse->arranger->controllerChanged(node);
      }

//---------------------------------------------------------
//   msgSetPrefader
//---------------------------------------------------------

void Audio::msgSetPrefader(AudioTrack* node, int val)
      {
      AudioMsg msg;
      msg.id    = AUDIO_SET_PREFADER;
      msg.snode = node;
      msg.ival  = val;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetChannels
//---------------------------------------------------------

void Audio::msgSetChannels(AudioTrack* node, int n)
      {
      if (n == node->channels())
            return;
      QString name = node->name();
      int mc       = std::max(n, node->channels());

      if (!name.isEmpty()) 
      {
            if (node->type() == Track::AUDIO_INPUT) 
            {
              if (!checkAudioDevice()) return;
              AudioInput* ai = (AudioInput*)node;
              for (int i = 0; i < mc; ++i) 
              {
                if (i < n && ai->jackPort(i) == 0) 
                {
                  char buffer[128];
                  snprintf(buffer, 128, "%s-%d", name.latin1(), i);
                  //ai->setJackPort(i, audioDevice->registerInPort(buffer));
                  ai->setJackPort(i, audioDevice->registerInPort(buffer, false));
                }
                else if ((i >= n) && ai->jackPort(i)) 
                {
                  RouteList* ir = node->inRoutes();
                  for (iRoute ii = ir->begin(); ii != ir->end(); ++ii) 
                  {
                    Route r = *ii;
                    if ((r.type == Route::JACK_ROUTE) && (r.channel == i)) 
                    {
                      msgRemoveRoute(r, Route(node,i));
                      break;
                    }
                  }
                  audioDevice->unregisterPort(ai->jackPort(i));
                  ai->setJackPort(i, 0);
                }
              }      
            }
            else if (node->type() == Track::AUDIO_OUTPUT) 
            {
                  if (!checkAudioDevice()) return;
                  AudioOutput* ao = (AudioOutput*)node;
                  for (int i = 0; i < mc; ++i) 
                  {
                        void* jp = ao->jackPort(i);
                        if (i < n && jp == 0) 
                        {
                              char buffer[128];
                              snprintf(buffer, 128, "%s-%d", name.latin1(), i);
                              //ao->setJackPort(i, audioDevice->registerOutPort(buffer));
                              ao->setJackPort(i, audioDevice->registerOutPort(buffer, false));
                        }
                        else if (i >= n && jp) 
                        {
                              RouteList* ir = node->outRoutes();
                              for (iRoute ii = ir->begin(); ii != ir->end(); ++ii) 
                              {
                                    Route r = *ii;
                                    if ((r.type == Route::JACK_ROUTE) && (r.channel == i)) 
                                    {
                                          msgRemoveRoute(Route(node,i), r);
                                          break;
                                    }
                              }
                              audioDevice->unregisterPort(jp);
                              ao->setJackPort(i, 0);
                        }
                  }
            }
      }      
      
      /* TODO TODO: Change all stereo routes to mono. 
      // If we are going from stereo to mono we need to disconnect any stray synti 'mono last channel'...
      if(n == 1 && node->channels() > 1)
      {
        // This should always happen - syntis are fixed channels, user cannot change them. But to be safe...
        if(node->type() != Track::AUDIO_SOFTSYNTH) 
        {
          if(node->type() != Track::AUDIO_INPUT) 
          {
            RouteList* rl = node->inRoutes();
            for(iRoute r = rl->begin(); r != rl->end(); ++r)
            {
              // Only interested in synth tracks.
              if(r->type != Route::TRACK_ROUTE || r->track->type() != Track::AUDIO_SOFTSYNTH)
                continue;  
              // If it's the last channel...
              if(r->channel + 1 == ((AudioTrack*)r->track)->totalOutChannels())
              {
                msgRemoveRoute(*r, Route(node, r->channel));
                //msgRemoveRoute(r, Route(node, r->remoteChannel));
                break;
              }
            }
          }  
        
          if(node->type() != Track::AUDIO_OUTPUT) 
          {
            RouteList* rl = node->outRoutes();
            for(iRoute r = rl->begin(); r != rl->end(); ++r)
            {
              // Only interested in synth tracks.
              if(r->type != Route::TRACK_ROUTE || r->track->type() != Track::AUDIO_SOFTSYNTH)
                continue;  
              // If it's the last channel...
              if(r->channel + 1 == ((AudioTrack*)r->track)->totalOutChannels())
              {
                msgRemoveRoute(Route(node, r->channel), *r);
                //msgRemoveRoute(Route(node, r->remoteChannel), r);
                break;
              }
            }
          }  
        }   
      }
      */        
              
      AudioMsg msg;
      msg.id    = AUDIO_SET_CHANNELS;
      msg.snode = node;
      msg.ival  = n;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetPluginCtrlVal
//---------------------------------------------------------

//void Audio::msgSetPluginCtrlVal(PluginI* plugin, int param, double val)
// p3.3.43
void Audio::msgSetPluginCtrlVal(AudioTrack* track, int param, double val)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SET_PLUGIN_CTRL_VAL;
      msg.ival   = param;
      msg.dval   = val;
      //msg.plugin = plugin;
      msg.snode  = track;
      sendMsg(&msg);
      //muse->arranger->controllerChanged(track);
}

//---------------------------------------------------------
//   msgSwapControllerIDX
//---------------------------------------------------------

void Audio::msgSwapControllerIDX(AudioTrack* node, int idx1, int idx2)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SWAP_CONTROLLER_IDX;
      msg.snode  = node;
      msg.a      = idx1;
      msg.b      = idx2;
      sendMsg(&msg);
      //muse->arranger->controllerChanged(node);
}

//---------------------------------------------------------
//   msgClearControllerEvents
//---------------------------------------------------------

void Audio::msgClearControllerEvents(AudioTrack* node, int acid)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_CLEAR_CONTROLLER_EVENTS;
      msg.snode  = node;
      msg.ival   = acid;
      sendMsg(&msg);
      //muse->arranger->controllerChanged(node);
}

//---------------------------------------------------------
//   msgSeekPrevACEvent
//---------------------------------------------------------

void Audio::msgSeekPrevACEvent(AudioTrack* node, int acid)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SEEK_PREV_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgSeekNextACEvent
//---------------------------------------------------------

void Audio::msgSeekNextACEvent(AudioTrack* node, int acid)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SEEK_NEXT_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgEraseACEvent
//---------------------------------------------------------

void Audio::msgEraseACEvent(AudioTrack* node, int acid, int frame)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_ERASE_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      msg.a      = frame; 
      sendMsg(&msg);
      //muse->arranger->controllerChanged(node);
}

//---------------------------------------------------------
//   msgEraseRangeACEvents
//---------------------------------------------------------

void Audio::msgEraseRangeACEvents(AudioTrack* node, int acid, int frame1, int frame2)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_ERASE_RANGE_AC_EVENTS;
      msg.snode  = node;
      msg.ival   = acid;
      msg.a      = frame1; 
      msg.b      = frame2; 
      sendMsg(&msg);
      //muse->arranger->controllerChanged(node);
}

//---------------------------------------------------------
//   msgAddACEvent
//---------------------------------------------------------

void Audio::msgAddACEvent(AudioTrack* node, int acid, int frame, double val)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_ADD_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      msg.a      = frame; 
      msg.dval   = val;
      sendMsg(&msg);
      //muse->arranger->controllerChanged(node);
}

//---------------------------------------------------------
//   msgSetSolo
//---------------------------------------------------------

void Audio::msgSetSolo(Track* track, bool val)
{
      AudioMsg msg;
      msg.id     = AUDIO_SET_SOLO;
      msg.track  = track;
      msg.ival   = int(val);
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgSetSegSize
//---------------------------------------------------------

void Audio::msgSetSegSize(int bs, int sr)
      {
      AudioMsg msg;
      msg.id = AUDIO_SET_SEG_SIZE;
      msg.ival = bs;
      msg.iival = sr;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSeek
//---------------------------------------------------------

void Audio::msgSeek(const Pos& pos)
      {
      if (!checkAudioDevice()) return;
      //audioDevice->seekTransport(pos.frame());
            // p3.3.23
            //printf("Audio::msgSeek before audioDevice->seekTransport frame:%d\n", pos.frame());
      audioDevice->seekTransport(pos);
            // p3.3.23
            //printf("Audio::msgSeek after audioDevice->seekTransport frame:%d\n", pos.frame());
      }

//---------------------------------------------------------
//   msgUndo
//---------------------------------------------------------

void Audio::msgUndo()
      {
      AudioMsg msg;
      msg.id = SEQM_UNDO;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgRedo
//---------------------------------------------------------

void Audio::msgRedo()
      {
      AudioMsg msg;
      msg.id = SEQM_REDO;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgPlay
//---------------------------------------------------------

void Audio::msgPlay(bool val)
      {
      if (val) {
            if (audioDevice)
            {
                unsigned sfr = song->cPos().frame();
                unsigned dcfr = audioDevice->getCurFrame();
                if(dcfr != sfr)
                  //audioDevice->seekTransport(sfr);
                  audioDevice->seekTransport(song->cPos());
                audioDevice->startTransport();
            }
              
      }else {
            if (audioDevice)
                audioDevice->stopTransport();
            _bounce = false;
            }
      }

//---------------------------------------------------------
//   msgShowInstrumentGui
//---------------------------------------------------------

void Audio::msgShowInstrumentGui(MidiInstrument* instr, bool val)
      {
      instr->showGui(val);
      AudioMsg msg;
      msg.id = MIDI_SHOW_INSTR_GUI;
      msg.p1 = instr;
      msg.a  = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgAddTrack
//---------------------------------------------------------

void Song::msgInsertTrack(Track* track, int idx, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_TRACK;
      msg.track = track;
      msg.ival  = idx;
      if (doUndoFlag) {
            song->startUndo();
            undoOp(UndoOp::AddTrack, idx, track);
            }
      audio->sendMsg(&msg);
      if (doUndoFlag)
            endUndo(SC_TRACK_INSERTED);
      }

//---------------------------------------------------------
//   msgRemoveTrack
//---------------------------------------------------------

void Audio::msgRemoveTrack(Track* track, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_TRACK;
      msg.track = track;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveTracks
//    remove all selected tracks
//---------------------------------------------------------

void Audio::msgRemoveTracks()
{
      bool loop;
      do 
      {
        loop = false;
        TrackList* tl = song->tracks();
        for (iTrack t = tl->begin(); t != tl->end(); ++t) 
        {
          Track* tr = *t;
          if (tr->selected()) 
          {
            song->removeTrack1(tr);
            msgRemoveTrack(tr, false);
            song->removeTrack3(tr);
            loop = true;
            break;
          }
        }
      } 
      while (loop);
            
     /*       
            // TESTED: DIDN'T WORK: It still skipped some selected tracks !
            // Quote from SGI STL: "Erasing an element from a map also does not invalidate any iterators, 
            //                      except, of course, for iterators that actually point to the element 
            //                      that is being erased."
            // Well that doesn't seem true here...
            
            TrackList* tl = song->tracks();
            for(ciTrack t = tl->begin(); t != tl->end() ; ) 
            {
                  if((*t)->selected()) 
                  {
                        // Changed 20070102: - Iterator t becomes invalid after msgRemoveTrack.
                        ciTrack tt = t;
                        ++t;
                        Track* tr = *tt;
                        
                        song->removeTrack1(tr);
                        msgRemoveTrack(tr, false);
                        song->removeTrack3(tr);
                        
                  }
                  else
                    ++t;
                        
            }
  */          
            
}

//---------------------------------------------------------
//   msgChangeTrack
//    oldTrack - copy of the original track befor modification
//    newTrack - modified original track
//---------------------------------------------------------

void Audio::msgChangeTrack(Track* oldTrack, Track* newTrack, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_CHANGE_TRACK;
      msg.p1 = oldTrack;
      msg.p2 = newTrack;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgMoveTrack
//    move track idx1 to slot idx2
//---------------------------------------------------------

void Audio::msgMoveTrack(int idx1, int idx2, bool doUndoFlag)
      {
      if (idx1 < 0 || idx2 < 0)   // sanity check
            return;
      int n = song->tracks()->size();
      if (idx1 >= n || idx2 >= n)   // sanity check
            return;
      AudioMsg msg;
      msg.id = SEQM_MOVE_TRACK;
      msg.a = idx1;
      msg.b = idx2;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddPart
//---------------------------------------------------------

void Audio::msgAddPart(Part* part, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_PART;
      msg.p1 = part;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemovePart
//---------------------------------------------------------

void Audio::msgRemovePart(Part* part, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_PART;
      msg.p1 = part;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveParts
//    remove selected parts; return true if any part was
//    removed
//---------------------------------------------------------

bool Song::msgRemoveParts()
      {
      bool loop;
      bool partSelected = false;
      do {
            loop = false;
            TrackList* tl = song->tracks();

            for (iTrack it = tl->begin(); it != tl->end(); ++it) {
                  PartList* pl = (*it)->parts();
                  for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                        if (ip->second->selected()) {
                              if ((*it)->type() == Track::WAVE) {
                                    audio->msgRemovePart((WavePart*)(ip->second));
                                    }
                              else {
                                    audio->msgRemovePart(ip->second, false);
                                    }
                              loop = true;
                              partSelected = true;
                              break;
                              }
                        }
                  if (loop)
                        break;
                  }
            } while (loop);
      return partSelected;
      }

//---------------------------------------------------------
//   msgChangePart
//---------------------------------------------------------

//void Audio::msgChangePart(Part* oldPart, Part* newPart, bool doUndoFlag)
void Audio::msgChangePart(Part* oldPart, Part* newPart, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      AudioMsg msg;
      msg.id = SEQM_CHANGE_PART;
      msg.p1 = oldPart;
      msg.p2 = newPart;
      msg.a = doCtrls;
      msg.b = doClones;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddEvent
//---------------------------------------------------------

//void Audio::msgAddEvent(Event& event, Part* part, bool doUndoFlag)
void Audio::msgAddEvent(Event& event, Part* part, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_EVENT;
      msg.ev1 = event;
      msg.p2 = part;
      msg.a = doCtrls;
      msg.b = doClones;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgDeleteEvent
//---------------------------------------------------------

//void Audio::msgDeleteEvent(Event& event, Part* part, bool doUndoFlag)
void Audio::msgDeleteEvent(Event& event, Part* part, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_EVENT;
      msg.ev1 = event;
      msg.p2 = part;
      msg.a = doCtrls;
      msg.b = doClones;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgChangeEvent
//---------------------------------------------------------

//void Audio::msgChangeEvent(Event& oe, Event& ne, Part* part, bool doUndoFlag)
void Audio::msgChangeEvent(Event& oe, Event& ne, Part* part, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      AudioMsg msg;
      msg.id = SEQM_CHANGE_EVENT;
      msg.ev1 = oe;
      msg.ev2 = ne;
      msg.p3 = part;
      msg.a = doCtrls;
      msg.b = doClones;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddTempo
//---------------------------------------------------------

void Audio::msgAddTempo(int tick, int tempo, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_TEMPO;
      msg.a = tick;
      msg.b = tempo;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgSetTempo
//---------------------------------------------------------

void Audio::msgSetTempo(int tick, int tempo, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_TEMPO;
      msg.a = tick;
      msg.b = tempo;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgSetGlobalTempo
//---------------------------------------------------------

void Audio::msgSetGlobalTempo(int val)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_GLOBAL_TEMPO;
      msg.a = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgDeleteTempo
//---------------------------------------------------------

void Audio::msgDeleteTempo(int tick, int tempo, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_TEMPO;
      msg.a = tick;
      msg.b = tempo;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddSig
//---------------------------------------------------------

void Audio::msgAddSig(int tick, int z, int n, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_SIG;
      msg.a = tick;
      msg.b = z;
      msg.c = n;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveSig
//! sends remove tempo signature message
//---------------------------------------------------------

void Audio::msgRemoveSig(int tick, int z, int n, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_SIG;
      msg.a = tick;
      msg.b = z;
      msg.c = n;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgScanAlsaMidiPorts
//---------------------------------------------------------

void Audio::msgScanAlsaMidiPorts()
      {
      AudioMsg msg;
      msg.id = SEQM_SCAN_ALSA_MIDI_PORTS;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgResetMidiDevices
//---------------------------------------------------------

void Audio::msgResetMidiDevices()
      {
      AudioMsg msg;
      msg.id = SEQM_RESET_DEVICES;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgInitMidiDevices
//---------------------------------------------------------

void Audio::msgInitMidiDevices()
      {
      AudioMsg msg;
      msg.id = SEQM_INIT_DEVICES;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Audio::msgPanic()
      {
      AudioMsg msg;
      msg.id = SEQM_PANIC;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void Audio::msgLocalOff()
      {
      AudioMsg msg;
      msg.id = SEQM_MIDI_LOCAL_OFF;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgUpdateSoloStates
//---------------------------------------------------------

void Audio::msgUpdateSoloStates()
      {
      AudioMsg msg;
      msg.id    = SEQM_UPDATE_SOLO_STATES;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetAux
//---------------------------------------------------------

void Audio::msgSetAux(AudioTrack* track, int idx, double val)
      {
      AudioMsg msg;
      msg.id    = SEQM_SET_AUX;
      msg.snode = track;
      msg.ival  = idx;
      msg.dval  = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgPlayMidiEvent
//---------------------------------------------------------

void Audio::msgPlayMidiEvent(const MidiPlayEvent* event)
      {
      AudioMsg msg;
      msg.id = SEQM_PLAY_MIDI_EVENT;
      msg.p1 = event;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgSetHwCtrlState
//---------------------------------------------------------

void Audio::msgSetHwCtrlState(MidiPort* port, int ch, int ctrl, int val)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_HW_CTRL_STATE;
      msg.p1 = port;
      msg.a = ch;
      msg.b = ctrl;
      msg.c = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgSetHwCtrlState
//---------------------------------------------------------

void Audio::msgSetHwCtrlStates(MidiPort* port, int ch, int ctrl, int val, int lastval)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_HW_CTRL_STATE;
      msg.p1 = port;
      msg.a = ch;
      msg.b = ctrl;
      msg.c = val;
      msg.ival = lastval;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgSetTrackOutChannel
//---------------------------------------------------------

void Audio::msgSetTrackOutChannel(MidiTrack* track, int ch)
{
      AudioMsg msg;
      msg.id = SEQM_SET_TRACK_OUT_CHAN;
      msg.p1 = track;
      msg.a = ch;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgSetTrackOutPort
//---------------------------------------------------------

void Audio::msgSetTrackOutPort(MidiTrack* track, int port)
{
      AudioMsg msg;
      msg.id = SEQM_SET_TRACK_OUT_PORT;
      msg.p1 = track;
      msg.a = port;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgRemapPortDrumCtlEvents
//---------------------------------------------------------

void Audio::msgRemapPortDrumCtlEvents(int mapidx, int newnote, int newchan, int newport)
{
      AudioMsg msg;
      msg.id = SEQM_REMAP_PORT_DRUM_CTL_EVS;
      msg.ival = mapidx;
      msg.a = newnote;
      msg.b = newchan;
      msg.c = newport;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgChangeAllPortDrumCtlEvents
//---------------------------------------------------------

void Audio::msgChangeAllPortDrumCtrlEvents(bool add, bool drumonly)
{
      AudioMsg msg;
      msg.id = SEQM_CHANGE_ALL_PORT_DRUM_CTL_EVS;
      msg.a = (int)add;
      msg.b = (int)drumonly;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgSetSendMetronome
//---------------------------------------------------------

void Audio::msgSetSendMetronome(AudioTrack* track, bool b)
{
      AudioMsg msg;
      msg.id    = AUDIO_SET_SEND_METRONOME;
      msg.snode = track;
      msg.ival  = (int)b;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgBounce
//    start bounce operation
//---------------------------------------------------------

void Audio::msgBounce()
      {
      _bounce = true;
      if (!checkAudioDevice()) return;
      //audioDevice->seekTransport(song->lPos().frame());
      audioDevice->seekTransport(song->lPos());
      }

//---------------------------------------------------------
//   msgIdle
//---------------------------------------------------------

void Audio::msgIdle(bool on)
      {
      AudioMsg msg;
      msg.id = SEQM_IDLE;
      msg.a  = on;
      sendMessage(&msg, false);
      }

