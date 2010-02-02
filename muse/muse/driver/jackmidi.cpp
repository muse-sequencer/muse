//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: jackmidi.cpp,v 1.1.1.1 2010/01/27 09:06:43 terminator356 Exp $
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "jackmidi.h"
#include "globals.h"
#include "midi.h"
#include "mididev.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
#include "mpevent.h"
//#include "sync.h"

int jackmidi_pi[2];
int jackmidi_po[2];

extern muse_jack_midi_buffer jack_midi_out_data[JACK_MIDI_CHANNELS];
extern muse_jack_midi_buffer jack_midi_in_data[JACK_MIDI_CHANNELS];

MidiJackDevice* gmdev = NULL;

int* jackSeq;
//static snd_seq_addr_t musePort;

//---------------------------------------------------------
//   MidiAlsaDevice
//---------------------------------------------------------

MidiJackDevice::MidiJackDevice(const int& a, const QString& n)
   : MidiDevice(n)
{
  adr = a;
  init();
}

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

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString MidiJackDevice::open()
{
  _readEnable = true;
  _writeEnable = true;


  return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void MidiJackDevice::close()
{
  _readEnable = false;
  _writeEnable = false;
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

/* FIX: if we fail to transmit the event,
 *      we return false (indicating OK). Otherwise
 *      it seems muse will retry forever
 */
bool MidiJackDevice::putMidiEvent(const MidiPlayEvent& event)
{
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
  
  /*
  //
  // NOTICE: Only one MusE port (port 0) is supported for now ! MusE has no mechanism to create 
  //  or select other MusE ports. MusE ALSA midi only creates one port as well.
  //
  const int museport = 0;
  if(event.type() == ME_CONTROLLER) 
  {
    int a      = event.dataA();
    int b      = event.dataB();
    int chn    = event.channel();
    unsigned t = event.time();

    if(a == CTRL_PITCH) 
    {
      int v = b + 8192;
      audioDriver->putEvent(museport, MidiPlayEvent(t, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f));
    }
    else if (a == CTRL_PROGRAM) 
    {
      // don't output program changes for GM drum channel
      //if (!(song->mtype() == MT_GM && chn == 9)) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0x7f;
            if (hb != 0xff)
                  audioDriver->putEvent(museport, MidiPlayEvent(t, chn, ME_CONTROLLER, CTRL_HBANK, hb));
            if (lb != 0xff)
                  audioDriver->putEvent(museport, MidiPlayEvent(t+1, chn, ME_CONTROLLER, CTRL_LBANK, lb));
            audioDriver->putEvent(museport, MidiPlayEvent(t+2, chn, ME_PROGRAM, pr, 0));
      //      }
    }
    else if (a == CTRL_MASTER_VOLUME) 
    {
      unsigned char sysex[] = {
            0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
            };
      sysex[1] = deviceId();
      sysex[4] = b & 0x7f;
      sysex[5] = (b >> 7) & 0x7f;
      audioDriver->putEvent(museport, MidiPlayEvent(t, ME_SYSEX, sysex, 6));
    }
    else if (a < CTRL_14_OFFSET) 
    {              // 7 Bit Controller
      audioDriver->putEvent(museport, event);
    }
    else if (a < CTRL_RPN_OFFSET) 
    {     // 14 bit high resolution controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      audioDriver->putEvent(museport, MidiPlayEvent(t,   chn, ME_CONTROLLER, ctrlH, dataH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+1, chn, ME_CONTROLLER, ctrlL, dataL));
    }
    else if (a < CTRL_NRPN_OFFSET) 
    {     // RPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      audioDriver->putEvent(museport, MidiPlayEvent(t,   chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+1, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
      audioDriver->putEvent(museport, MidiPlayEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, b));
    }
    else if (a < CTRL_RPN14_OFFSET) 
    {     // NRPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      audioDriver->putEvent(museport, MidiPlayEvent(t,   chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+1, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
      audioDriver->putEvent(museport, MidiPlayEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, b));
    }
    else if (a < CTRL_NRPN14_OFFSET) 
    {     // RPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      audioDriver->putEvent(museport, MidiPlayEvent(t,   chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+1, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
      audioDriver->putEvent(museport, MidiPlayEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+3, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
    }
    else if (a < CTRL_NONE_OFFSET) 
    {     // NRPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      audioDriver->putEvent(museport, MidiPlayEvent(t, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+1, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
      audioDriver->putEvent(museport, MidiPlayEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
      audioDriver->putEvent(museport, MidiPlayEvent(t+3, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
    }
    else 
    {
      printf("MidiJackDevice::putMidiEvent: unknown controller type 0x%x\n", a);
    }
  }
  else 
  {
    audioDriver->putEvent(museport, event);
  }
  
  // Just return OK for now.
  return false;
  */
  
}

//---------------------------------------------------------
//   putEvent
//    return false if event is delivered
//---------------------------------------------------------

bool MidiJackDevice::putEvent(int* event)
{
  int *y; y = event;
  return false;
}

//---------------------------------------------------------
//   initMidiJack
//    return true on error
//---------------------------------------------------------

bool initMidiJack()
{
  int adr = 0;

  memset(jack_midi_out_data, 0, JACK_MIDI_CHANNELS * sizeof(muse_jack_midi_buffer));
  memset(jack_midi_in_data, 0, JACK_MIDI_CHANNELS * sizeof(muse_jack_midi_buffer));

  MidiJackDevice* dev = new MidiJackDevice(adr, QString("jack-midi"));
  dev->setrwFlags(3); /* set read and write flags */
  if(pipe(jackmidi_pi) < 0){
    fprintf(stderr, "cant create midi-jack input pipe\n");
  }
  if(pipe(jackmidi_po) < 0){
    fprintf(stderr, "cant create midi-jack output pipe\n");
  }
  midiDevices.add(dev);
  gmdev = dev; /* proclaim the global jack-midi instance */

  return false;
}

struct JackPort {
      int adr;
      char* name;
      int flags;
      JackPort(int a, const char* s, int f) {
            adr = a;
            name = strdup(s);
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

  if(t == 0x90){ /* note on */
    fprintf(stderr, "jackProcessMidiInput note-on\n");
    event.setChannel(channel);
    event.setType(ME_NOTEON);
    event.setA(n);
    event.setB(v);
  }else if (t == 0x80){ /* note off */
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

