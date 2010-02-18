//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: jackmidi.cpp,v 1.1.1.1 2010/01/27 09:06:43 terminator356 Exp $
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//=========================================================

#include <qstring.h>
#include <stdio.h>

#include <jack/jack.h>
//#include <jack/midiport.h>

#include "jackmidi.h"
#include "song.h"
#include "globals.h"
#include "midi.h"
#include "mididev.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
#include "mpevent.h"
//#include "sync.h"
#include "audiodev.h"
#include "../mplugins/midiitransform.h"
#include "../mplugins/mitplugin.h"

// Turn on debug messages.
//#define JACK_MIDI_DEBUG

///int jackmidi_pi[2];
///int jackmidi_po[2];

//extern muse_jack_midi_buffer jack_midi_out_data[JACK_MIDI_CHANNELS];
//extern muse_jack_midi_buffer jack_midi_in_data[JACK_MIDI_CHANNELS];
///extern jack_port_t *midi_port_in[JACK_MIDI_CHANNELS];
///extern jack_port_t *midi_port_out[JACK_MIDI_CHANNELS];

///MidiJackDevice* gmdev = NULL;

///int* jackSeq;
//static snd_seq_addr_t musePort;

int MidiJackDevice::_nextOutIdNum = 0;
int MidiJackDevice::_nextInIdNum = 0;

//---------------------------------------------------------
//   MidiAlsaDevice
//---------------------------------------------------------

MidiJackDevice::MidiJackDevice(const int& a, const QString& n)
   : MidiDevice(n)
{
  _client_jackport = 0;
  adr = a;
  init();
}

MidiJackDevice::~MidiJackDevice()
{
  #ifdef JACK_MIDI_USE_MULTIPLE_CLIENT_PORTS    
  if(_client_jackport)
    //audioDevice->unregisterPort(_client_jackport);
    close();
  #endif
}
            
/*
//---------------------------------------------------------
//   select[RW]fd
//---------------------------------------------------------

int MidiJackDevice::selectRfd()
{
  return jackmidi_pi[0];
}

int MidiJackDevice::selectWfd()
{
  return jackmidi_po[0];
}
*/

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString MidiJackDevice::open()
{
  _openFlags &= _rwFlags; // restrict to available bits
  
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::open %s\n", name.latin1());
  #endif  
  
  //jack_port_t* jp = jack_port_by_name(_client, name().latin1());
  jack_port_t* jp = (jack_port_t*)audioDevice->findPort(name().latin1());
  
  if(!jp)
  {
    printf("MidiJackDevice::open: Jack midi port %s not found!\n", name().latin1());
    _writeEnable = false;
    _readEnable = false;
    return QString("Jack midi port not found");
  }
    
  int pf = jack_port_flags(jp);
  
  // If Jack port can receive data from us and we actually want to...
  if((pf & JackPortIsInput) && (_openFlags & 1))
  {
    char buf[80];
    snprintf(buf, 80, "muse-jack-midi-out-%d", _nextOutIdNum);
    _client_jackport = (jack_port_t*)audioDevice->registerOutPort(buf, true);
    if(_client_jackport == NULL)
    {
      fprintf(stderr, "MidiJackDevice::open failed to register jack-midi-out\n");
      _writeEnable = false;
      return QString("Could not register jack-midi-out client port");
    }
    else
    {
      _nextOutIdNum++;
      // src, dest
      ///audioDevice->connect(_client_jackport, jp);
      _writeEnable = true;
    }
  }
  else // Note docs say it can't be both input and output.
  // If Jack port can send data to us and we actually want it...
  if((pf & JackPortIsOutput) && (_openFlags & 2))
  {  
    char buf[80];
    snprintf(buf, 80, "muse-jack-midi-in-%d", _nextInIdNum);
    _client_jackport = (jack_port_t*)audioDevice->registerInPort(buf, true);
    if(_client_jackport == NULL)
    {
      fprintf(stderr, "MidiJackDevice::open failed to register jack-midi-in\n");
      _readEnable = false;
      return QString("Could not register jack-midi-in client port");
    }
    else
    {
      _nextInIdNum++;
      ///audioDevice->connect(jp, _client_jackport);
      _readEnable = true;
    }
  }
  return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void MidiJackDevice::close()
{
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::close %s\n", name.latin1());
  #endif  
  
  if(_client_jackport)
  {
    int pf = jack_port_flags(_client_jackport);

    if(pf & JackPortIsOutput)
      _nextOutIdNum--;
    else
    if(pf & JackPortIsInput)
      _nextInIdNum--;
    audioDevice->unregisterPort(_client_jackport);
    _client_jackport = 0;
    _writeEnable = false;
    _readEnable = false;
    return;
  }  
    
  /*
  //jack_port_t* jp = jack_port_by_name(_client, name().latin1());
  jack_port_t* jp = (jack_port_t*)audioDevice->findPort(name().latin1());
  
  if(!jp)
  {
    printf("MidiJackDevice::close: Jack midi port %s not found!\n", name().latin1());
    _writeEnable = false;
    _readEnable = false;
    return;
  }
    
  //int pf = jack_port_flags(jp);
  
  // If Jack port can receive data from us and we actually want to...
  //if((pf & JackPortIsInput) && (_openFlags & 1))
  if(jack_port_connected_to(midi_port_out[0], name().latin1()))
  {
    // src, dest
///    audioDevice->disconnect(midi_port_out[0], jp);
    _writeEnable = false;
  }
  else // Note docs say it can't be both input and output.  
  // If Jack port can send data to us and we actually want it...
  //if((pf & JackPortIsOutput) && (_openFlags & 2))
  if(jack_port_connected_to(midi_port_in[0], name().latin1()))
  {  
///    audioDevice->disconnect(jp, midi_port_in[0]);
    _readEnable = false;
  }
  */
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

/* FIX: if we fail to transmit the event,
 *      we return false (indicating OK). Otherwise
 *      it seems muse will retry forever
 */
bool MidiJackDevice::putMidiEvent(const MidiPlayEvent& /*event*/)
{
  /*
  int give, channel = event.channel();
  int x;

  if(channel >= JACK_MIDI_CHANNELS) return false;

  // buffer up events, because jack eats them in chunks, if
   // the buffer is full, there isn't so much to do, than
   // drop the event
   
  give = jack_midi_out_data[channel].give;
  if(jack_midi_out_data[channel].buffer[give*4+3]){
    fprintf(stderr, "WARNING: muse-to-jack midi-buffer is full, channel=%u\n", channel);
    return false;
  }
  // copy event(note-on etc..), pitch and volume 
  // see http://www.midi.org/techspecs/midimessages.php 
  switch(event.type()){
    case ME_NOTEOFF:
      jack_midi_out_data[channel].buffer[give*4+0] = 0x80;
      jack_midi_out_data[channel].buffer[give*4+1] = event.dataA() & 0x7f;
      jack_midi_out_data[channel].buffer[give*4+2] = event.dataB() & 0x7f;
      break;
    case ME_NOTEON:
      jack_midi_out_data[channel].buffer[give*4+0] = 0x90;
      jack_midi_out_data[channel].buffer[give*4+1] = event.dataA() & 0x7f;
      jack_midi_out_data[channel].buffer[give*4+2] = event.dataB() & 0x7f;
      break;
    case ME_CONTROLLER:
      jack_midi_out_data[channel].buffer[give*4+0] = 0xb0;
      jack_midi_out_data[channel].buffer[give*4+1] = event.dataA() & 0x7f;
      jack_midi_out_data[channel].buffer[give*4+2] = event.dataB() & 0x7f;
      break;
    case ME_PROGRAM:
      jack_midi_out_data[channel].buffer[give*4+0] = 0xc0;
      jack_midi_out_data[channel].buffer[give*4+1] = event.dataA() & 0x7f;
      jack_midi_out_data[channel].buffer[give*4+2] = 0;
      break;
    case ME_PITCHBEND:
      jack_midi_out_data[channel].buffer[give*4+0] = 0xE0;
      // convert muse pitch-bend to midi standard 
      x = 0x2000 + event.dataA();
      jack_midi_out_data[channel].buffer[give*4+1] = x & 0x7f;
      jack_midi_out_data[channel].buffer[give*4+2] = (x >> 8) & 0x7f;
      break;
    default:
      fprintf(stderr, "jack-midi-out %u WARNING: unknown event %x\n", channel, event.type());
      return false;
  }
  jack_midi_out_data[channel].buffer[give*4+3] = 1; // mark state of this slot 
  // finally increase give position 
  give++;
  if(give >= JACK_MIDI_BUFFER_SIZE){
    give = 0;
  }
  jack_midi_out_data[channel].give = give;
  return false;
  */
  
  return false;
}

/*
//---------------------------------------------------------
//   putEvent
//    return false if event is delivered
//---------------------------------------------------------

bool MidiJackDevice::putEvent(int* event)
{
  int *y; y = event;
  return false;
}
*/

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiJackDevice::recordEvent(MidiRecordEvent& event)
      {
      // Set the loop number which the event came in at.
      //if(audio->isRecording())
      if(audio->isPlaying())
        event.setLoopNum(audio->loopCount());
      
      if (midiInputTrace) {
            printf("Jack MidiInput: ");
            event.dump();
            }

      if(_port != -1)
      {
        int idin = midiPorts[_port].syncInfo().idIn();
        
        int typ = event.type();
  
        //---------------------------------------------------
        // filter some SYSEX events
        //---------------------------------------------------
  
        if (typ == ME_SYSEX) {
              const unsigned char* p = event.data();
              int n = event.len();
              if (n >= 4) {
                    if ((p[0] == 0x7f)
                      //&& ((p[1] == 0x7f) || (p[1] == rxDeviceId))) {
                      && ((p[1] == 0x7f) || (idin == 0x7f) || (p[1] == idin))) {
                          if (p[2] == 0x06) {
                                //mmcInput(p, n);
                                midiSeq->mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                //mtcInputFull(p, n);
                                midiSeq->mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          //nonRealtimeSystemSysex(p, n);
                          midiSeq->nonRealtimeSystemSysex(_port, p, n);
                          return;
                          }
                    }
              }
          else    
            // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
            midiPorts[_port].syncInfo().trigActDetect(event.channel());
      }
      
      //
      //  process midi event input filtering and
      //    transformation
      //

      processMidiInputTransformPlugins(event);

      if (filterEvent(event, midiRecordType, false))
            return;
      
      if (!applyMidiInputTransformation(event)) {
            if (midiInputTrace)
                  printf("   midi input transformation: event filtered\n");
            return;
            }

      //
      // transfer noteOn events to gui for step recording and keyboard
      // remote control
      //
      if (event.type() == ME_NOTEON) {
            int pv = ((event.dataA() & 0xff)<<8) + (event.dataB() & 0xff);
            song->putEvent(pv);
            }
      
      if(_recordFifo.put(MidiPlayEvent(event)))
        printf("MidiJackDevice::recordEvent: fifo overflow\n");
      }

//---------------------------------------------------------
//   midiReceived
//---------------------------------------------------------

void MidiJackDevice::eventReceived(jack_midi_event_t* ev)
      {
      MidiRecordEvent event;
      event.setB(0);

      // NOTE: From MusE-2. Not done here in Muse-1 (yet).
      // move all events 2*segmentSize into the future to get
      // jitterfree playback
      //
      //  cycle   n-1         n          n+1
      //          -+----------+----------+----------+-
      //               ^          ^          ^
      //               catch      process    play
      //
//      const SeqTime* st = audio->seqTime();

      //unsigned curFrame = st->startFrame() + segmentSize;
//      unsigned curFrame = st->lastFrameTime;
      //int frameOffset = audio->getFrameOffset();
      unsigned pos = audio->pos().frame();
      event.setTime(pos + ev->time);

      event.setChannel(*(ev->buffer) & 0xf);
      int type = *(ev->buffer) & 0xf0;
      int a    = *(ev->buffer + 1) & 0x7f;
      int b    = *(ev->buffer + 2) & 0x7f;
      event.setType(type);
      switch(type) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_CONTROLLER:
                  event.setA(*(ev->buffer + 1));
                  event.setB(*(ev->buffer + 2));
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  event.setA(*(ev->buffer + 1));
                  break;

            case ME_PITCHBEND:
                  event.setA(((b << 7) + a) - 8192);
                  break;

            case ME_SYSEX:
                  {
                  int type = *(ev->buffer) & 0xff;
                  switch(type) {
                        case ME_SYSEX:
                              event.setTime(0);      // mark as used
                              event.setType(ME_SYSEX);
                              event.setData((unsigned char*)(ev->buffer + 1),
                                 ev->size - 2);
                              break;
                        case ME_CLOCK:
                        case ME_SENSE:
                              break;
                        default:
                              printf("MidiJackDevice::eventReceived unknown event 0x%02x\n", type);
                              return;
                        }
                  }
                  return;
            }

      if (midiInputTrace) {
            printf("MidiInput<%s>: ", name().latin1());
            event.dump();
            }
            
      #ifdef JACK_MIDI_DEBUG
      printf("MidiJackDevice::eventReceived time:%d type:%d ch:%d A:%d B:%d\n", event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      #endif  
      
      // Let recordEvent handle it from here, with timestamps, filtering, gui triggering etc.
      recordEvent(event);      
      }

//---------------------------------------------------------
//   collectMidiEvents
//---------------------------------------------------------

void MidiJackDevice::collectMidiEvents()
{
  if(!_readEnable)
    return;
  
  if(!_client_jackport)
    return;
  void* port_buf = jack_port_get_buffer(_client_jackport, segmentSize);
  
  jack_midi_event_t event;
  jack_nframes_t eventCount = jack_midi_get_event_count(port_buf);
  for (jack_nframes_t i = 0; i < eventCount; ++i) 
  {
    jack_midi_event_get(&event, port_buf, i);
    
    #ifdef JACK_MIDI_DEBUG
    printf("MidiJackDevice::collectMidiEvents number:%d time:%d\n", i, event.time);
    #endif  

    eventReceived(&event);
  }
}

//---------------------------------------------------------
//   putEvent
//    return true if event cannot be delivered
//---------------------------------------------------------

bool MidiJackDevice::putEvent(const MidiPlayEvent& ev)
{
  if(!_writeEnable)
    //return true;
    return false;
    
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::putEvent time:%d type:%d ch:%d A:%d B:%d\n", ev.time(), ev.type(), ev.channel(), ev.dataA(), ev.dataB());
  #endif  
      
  bool rv = eventFifo.put(ev);
  if(rv)
    printf("MidiJackDevice::putEvent: port overflow\n");
  
  return rv;
}

//---------------------------------------------------------
//   queueEvent
//   return true if successful
//---------------------------------------------------------

//void JackAudioDevice::putEvent(Port port, const MidiEvent& e)
bool MidiJackDevice::queueEvent(const MidiPlayEvent& e)
//bool MidiJackDevice::queueEvent(const MidiPlayEvent& e)
{
      // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
      // No big deal if not. Not used for now.
      //int port = e.port();
      
      //if(port >= JACK_MIDI_CHANNELS)
      //  return false;
        
      //if (midiOutputTrace) {
      //      printf("MidiOut<%s>: jackMidi: ", portName(port).toLatin1().data());
      //      e.dump();
      //      }
      
      //if(debugMsg)
      //  printf("MidiJackDevice::queueEvent\n");
    
      if(!_client_jackport)
        return false;
      void* pb = jack_port_get_buffer(_client_jackport, segmentSize);
    
      //unsigned frameCounter = ->frameTime();
      int frameOffset = audio->getFrameOffset();
      unsigned pos = audio->pos().frame();
      int ft = e.time() - frameOffset - pos;
      
      if (ft < 0)
            ft = 0;
      if (ft >= (int)segmentSize) {
            printf("MidiJackDevice::queueEvent: Event time:%d out of range. offset:%d ft:%d (seg=%d)\n", e.time(), frameOffset, ft, segmentSize);
            if (ft > (int)segmentSize)
                  ft = segmentSize - 1;
            }
      
      #ifdef JACK_MIDI_DEBUG
      printf("MidiJackDevice::queueEvent time:%d type:%d ch:%d A:%d B:%d\n", e.time(), e.type(), e.channel(), e.dataA(), e.dataB());
      #endif  
      
      switch(e.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
            case ME_PITCHBEND:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent note on/off polyafter controller or pitch\n");
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 3);
                  if (p == 0) {
                        fprintf(stderr, "MidiJackDevice::queueEvent #1: buffer overflow, event lost\n");
                        return false;
                        }
                  p[0] = e.type() | e.channel();
                  p[1] = e.dataA();
                  p[2] = e.dataB();
                  }
                  break;

            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent program or aftertouch\n");
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 2);
                  if (p == 0) {
                        fprintf(stderr, "MidiJackDevice::queueEvent #2: buffer overflow, event lost\n");
                        return false;
                        }
                  p[0] = e.type() | e.channel();
                  p[1] = e.dataA();
                  }
                  break;
            case ME_SYSEX:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent sysex\n");
                  #endif  
                  
                  const unsigned char* data = e.data();
                  int len = e.len();
                  unsigned char* p = jack_midi_event_reserve(pb, ft, len+2);
                  if (p == 0) {
                        fprintf(stderr, "MidiJackDevice::queueEvent #3: buffer overflow, event lost\n");
                        return false;
                        }
                  p[0] = 0xf0;
                  p[len+1] = 0xf7;
                  memcpy(p+1, data, len);
                  }
                  break;
            case ME_SONGPOS:
            case ME_CLOCK:
            case ME_START:
            case ME_CONTINUE:
            case ME_STOP:
                  printf("MidiJackDevice::queueEvent: event type %x not supported\n", e.type());
                  return false;
                  break;
            }
            
            return true;
}
      
//---------------------------------------------------------
//    processEvent
//---------------------------------------------------------

void MidiJackDevice::processEvent(const MidiPlayEvent& event)
{    
  //int frameOffset = audio->getFrameOffset();
  //unsigned pos = audio->pos().frame();

  int chn    = event.channel();
  unsigned t = event.time();
  
  // TODO: No sub-tick playback resolution yet, with external sync.
  // Just do this 'standard midi 64T timing thing' for now until we figure out more precise external timings. 
  // Does require relatively short audio buffers, in order to catch the resolution, but buffer <= 256 should be OK... 
  // Tested OK so far with 128. 
  if(extSyncFlag.value()) 
    t = audio->getFrameOffset() + audio->pos().frame();
    //t = frameOffset + pos;
      
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::processEvent time:%d type:%d ch:%d A:%d B:%d\n", event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
  #endif  
      
  if(event.type() == ME_CONTROLLER) 
  {
    int a      = event.dataA();
    int b      = event.dataB();
    // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
    // No big deal if not. Not used for now.
    int port   = event.port();

    int nvh = 0xff;
    int nvl = 0xff;
    if(_port != -1)
    {
      int nv = midiPorts[_port].nullSendValue();
      if(nv != -1)
      {
        nvh = (nv >> 8) & 0xff;
        nvl = nv & 0xff;
      }
    }
      
    if(a == CTRL_PITCH) 
    {
      int v = b + 8192;
      queueEvent(MidiPlayEvent(t, port, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f));
    }
    else if (a == CTRL_PROGRAM) 
    {
      // don't output program changes for GM drum channel
      //if (!(song->mtype() == MT_GM && chn == 9)) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0x7f;
            if (hb != 0xff)
                  queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HBANK, hb));
            if (lb != 0xff)
                  queueEvent(MidiPlayEvent(t+1, port, chn, ME_CONTROLLER, CTRL_LBANK, lb));
            queueEvent(MidiPlayEvent(t+2, port, chn, ME_PROGRAM, pr, 0));
      //      }
    }
    /*
    else if (a == CTRL_MASTER_VOLUME) 
    {
      unsigned char sysex[] = {
            0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
            };
      sysex[1] = deviceId();
      sysex[4] = b & 0x7f;
      sysex[5] = (b >> 7) & 0x7f;
      queueEvent(MidiPlayEvent(t, port, ME_SYSEX, sysex, 6));
    }
    */
    else if (a < CTRL_14_OFFSET) 
    {              // 7 Bit Controller
      queueEvent(event);
      //queueEvent(museport, MidiPlayEvent(t, port, chn, event));
    }
    else if (a < CTRL_RPN_OFFSET) 
    {     // 14 bit high resolution controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, ctrlH, dataH));
      queueEvent(MidiPlayEvent(t+1, port, chn, ME_CONTROLLER, ctrlL, dataL));
    }
    else if (a < CTRL_NRPN_OFFSET) 
    {     // RPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
      queueEvent(MidiPlayEvent(t+1, port, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
      queueEvent(MidiPlayEvent(t+2, port, chn, ME_CONTROLLER, CTRL_HDATA, b));
      
      t += 3;  
      // Select null parameters so that subsequent data controller events do not upset the last *RPN controller.
      //sendNullRPNParams(chn, false);
      if(nvh != 0xff)
      {
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HRPN, nvh & 0x7f));
        t += 1;  
      }
      if(nvl != 0xff)
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, nvl & 0x7f));
    }
    //else if (a < CTRL_RPN14_OFFSET) 
    else if (a < CTRL_INTERNAL_OFFSET) 
    {     // NRPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
      queueEvent(MidiPlayEvent(t+1, port, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
      queueEvent(MidiPlayEvent(t+2, port, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
      t += 3;  
      //sendNullRPNParams(chn, true);
      if(nvh != 0xff)
      {
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HNRPN, nvh & 0x7f));
        t += 1;  
      }
      if(nvl != 0xff)
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, nvl & 0x7f));
    }
    else if (a < CTRL_NRPN14_OFFSET) 
    {     // RPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
      queueEvent(MidiPlayEvent(t+1, port, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
      queueEvent(MidiPlayEvent(t+2, port, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
      queueEvent(MidiPlayEvent(t+3, port, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
      
      t += 4;  
      //sendNullRPNParams(chn, false);
      if(nvh != 0xff)
      {
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HRPN, nvh & 0x7f));
        t += 1;  
      }
      if(nvl != 0xff)
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, nvl & 0x7f));
    }
    else if (a < CTRL_NONE_OFFSET) 
    {     // NRPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
      queueEvent(MidiPlayEvent(t+1, port, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
      queueEvent(MidiPlayEvent(t+2, port, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
      queueEvent(MidiPlayEvent(t+3, port, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
    
      t += 4;  
      //sendNullRPNParams(chn, true);
      if(nvh != 0xff)
      {
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HNRPN, nvh & 0x7f));
        t += 1;  
      }
      if(nvl != 0xff)
        queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, nvl & 0x7f));
    }
    else 
    {
      printf("MidiJackDevice::processEvent: unknown controller type 0x%x\n", a);
    }
  }
  else 
  {
    queueEvent(event);
    //queueEvent(MidiPlayEvent(t, port, chn, event));
  }
}
    
//---------------------------------------------------------
//    processMidi called from audio process only.
//---------------------------------------------------------

void MidiJackDevice::processMidi()
{
  if(!_client_jackport)
    return;
  void* port_buf = jack_port_get_buffer(_client_jackport, segmentSize);
  jack_midi_clear_buffer(port_buf);
  
  while(!eventFifo.isEmpty())
  {
    MidiPlayEvent e(eventFifo.get());
    int evTime = e.time(); 
    // Is event marked to be played immediately?
    if(evTime == 0) 
    {
      // Nothing to do but stamp the event to be queued for frame 0+.
      //e.setTime(frameOffset + pos);
      e.setTime(audio->getFrameOffset() + audio->pos().frame());
    }
    
    #ifdef JACK_MIDI_DEBUG
    printf("MidiJackDevice::processMidi eventFifo time:%d type:%d ch:%d A:%d B:%d\n", e.time(), e.type(), e.channel(), e.dataA(), e.dataB());
    #endif  
    
    //el->insert(eventFifo.get());
    //el->insert(e);
    processEvent(e);
  }
  
  MPEventList* el = playEvents();
  if(el->empty())
    return;
  
  iMPEvent i = nextPlayEvent();
  for(; i != el->end(); ++i) 
  {
    processEvent(*i);
  }
  
  setNextPlayEvent(i);
}

//---------------------------------------------------------
//   initMidiJack
//    return true on error
//---------------------------------------------------------

bool initMidiJack()
{
  /*
  int adr = 0;

  memset(jack_midi_out_data, 0, JACK_MIDI_CHANNELS * sizeof(muse_jack_midi_buffer));
  memset(jack_midi_in_data, 0, JACK_MIDI_CHANNELS * sizeof(muse_jack_midi_buffer));

  MidiJackDevice* dev = new MidiJackDevice(adr, QString("jack-midi"));
  dev->setrwFlags(3); // set read and write flags 

  if(pipe(jackmidi_pi) < 0){
    fprintf(stderr, "cant create midi-jack input pipe\n");
  }
  if(pipe(jackmidi_po) < 0){
    fprintf(stderr, "cant create midi-jack output pipe\n");
  }
  
  midiDevices.add(dev);
  
  gmdev = dev; // proclaim the global jack-midi instance 

  //jackScanMidiPorts();
  */
  
  return false;
}

/*
struct JackPort {
      int adr;
      //char* name;
      QString name;
      int flags;
      //JackPort(int a, const char* s, int f) {
      JackPort(int a, const QString& s, int f) {
            adr = a;
            //name = strdup(s);
            name = QString(s);
            flags = f;
            }
      };


static std::list<JackPort> portList;

//---------------------------------------------------------
//   jackScanMidiPorts
//---------------------------------------------------------

void jackScanMidiPorts()
{
  int adr;
  const char* name;

  portList.clear();
  adr  = 0;
  name = strdup("namex");
  portList.push_back(JackPort(adr, name, 0));
  //
  //  check for devices to add
  //
  for (std::list<JackPort>::iterator k = portList.begin(); k != portList.end(); ++k) {
    iMidiDevice i = midiDevices.begin();
    for (;i != midiDevices.end(); ++i) {
      //MidiJackDevice* d = dynamic_cast<MidiJackDevice*>(*i);
      break;
      //if (d == 0) continue;
      //if ((k->adr.client == d->adr.client) && (k->adr.port == d->adr.port)) {
      //  break;
      //}
    }
    if (i == midiDevices.end()) {
      // add device
      MidiJackDevice* dev = new MidiJackDevice(k->adr, QString(k->name));
      dev->setrwFlags(k->flags);
      midiDevices.add(dev);
    }
  }
}
*/

/*
//---------------------------------------------------------
//   processInput
//---------------------------------------------------------
static void handle_jack_midi_in(int channel)
{
  MidiRecordEvent event;
  int t,n,v;
  t = jack_midi_in_data[channel].buffer[0];
  n = jack_midi_in_data[channel].buffer[1];
  v = jack_midi_in_data[channel].buffer[2];

  event.setType(0);      // mark as unused
  event.setPort(gmdev->midiPort());
  event.setB(0);

  if(t == 0x90){ // note on 
    fprintf(stderr, "jackProcessMidiInput note-on\n");
    event.setChannel(channel);
    event.setType(ME_NOTEON);
    event.setA(n);
    event.setB(v);
  }else if (t == 0x80){ // note off 
    fprintf(stderr, "jackProcessMidiInput note-off\n");
    event.setChannel(channel);
    event.setType(ME_NOTEOFF);
    event.setA(n);
    event.setB(v);
  }else{
    fprintf(stderr, "WARNING: unknown midi-in on channel %d: %x,%x,%x\n",
            channel, t, n, v);
    return;
  }
  if(event.type()){
    gmdev->recordEvent(event);
    midiPorts[gmdev->midiPort()].syncInfo().trigActDetect(event.channel());
  }
}

void MidiJackDevice::processInput()
{
  char buf;
  int i,s;
  read(gmdev->selectRfd(), &buf, 1);

  s = 1;
  for(i = 0; i < JACK_MIDI_CHANNELS; i++){
    if(jack_midi_in_data[i].buffer[3]){
      s = 0;
      handle_jack_midi_in(i);
      jack_midi_in_data[i].buffer[3] = 0;
    }
  }
}

*/