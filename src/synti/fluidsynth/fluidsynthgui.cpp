//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./synti/fluidsynth/fluidsynthgui.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
/*
 * MusE FLUID Synth softsynth plugin
 *
 * Copyright (C) 2004 Mathias Lundgren (lunar_shuttle@users.sourcforge.net)
 *
 * $Id: fluidsynthgui.cpp,v 1.13.2.2 2009/08/12 20:47:01 spamatica Exp $
 *
 */

#include "fluidsynthgui.h"
#include "fluidsynti.h"

#include <QFileDialog>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include "muse_math.h"

#include "muse/midi_consts.h"

#include "common_defs.h"

 /*
#include "muse/debug.h"
#include <iomanip>
#include <qtooltip.h>
#include <qapplication.h>
#include <qlistbox.h>
#define MUSE_FLUID_DEBUG      false
*/

FluidSynthGui::FluidSynthGui()
      : MessGui()
      {
      setWindowIcon(QIcon(":/fluidsynth0.png"));
      setupUi(this);
      channelListView->setRowCount(FS_MAX_NR_OF_CHANNELS);
      channelListView->setSelectionMode(QAbstractItemView::SingleSelection);
      QLabel *fluidLabel = new QLabel;
      fluidLabel->setPixmap(QIcon(":/fluidsynth1.png").pixmap(124, 45));
      FluidGrid->addWidget(fluidLabel, 2, 1, Qt::AlignHCenter);

      ChorusType->setItemIcon(0, QIcon(":/sine.xpm"));
      ChorusType->setItemIcon(1, QIcon(":/saw.xpm"));

      connect(this->getGuiSignal(),SIGNAL(wakeup()),this,SLOT(readMessage()));
      connect (Push, SIGNAL (clicked()), SLOT(loadClicked()));

      lastdir = "";
      
      ReverbFrame->setEnabled(true);
      ChorusFrame->setEnabled(true);

      if (!FS_DEBUG)
            dumpInfoButton->hide();

      //Init reverb sliders:
      /*ReverbRoomSize->setValue((int)(16383*FS_PREDEF_REVERB_ROOMSIZE));
      ReverbDamping->setValue((int)(16383*FS_PREDEF_REVERB_DAMPING));
      ReverbWidth->setValue((int)(16383*FS_PREDEF_REVERB_WIDTH));*/

      connect(Gain, SIGNAL(valueChanged(int)), SLOT(changeGain(int)));
      connect(dumpInfoButton	, SIGNAL(clicked()), SLOT(dumpInfo()));
      connect(channelListView, SIGNAL(itemClicked(QTableWidgetItem*)),
         this, SLOT(channelItemClicked(QTableWidgetItem*)));

      connect(Reverb, SIGNAL (toggled(bool)), SLOT(toggleReverb(bool)));
      connect(ReverbLevel, SIGNAL (valueChanged (int)), SLOT(changeReverbLevel(int)));
      connect(ReverbRoomSize, SIGNAL (valueChanged (int)), SLOT(changeReverbRoomSize(int)));
      connect(ReverbDamping, SIGNAL (valueChanged (int)), SLOT(changeReverbDamping(int)));
      connect(ReverbWidth, SIGNAL (valueChanged (int)), SLOT(changeReverbWidth(int)));

      connect (Pop, SIGNAL (clicked()), SLOT(popClicked()));
      connect(sfListView, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
	 this, SLOT(sfItemClicked(QTreeWidgetItem*, int)));
      connect(Chorus, SIGNAL (toggled (bool)), SLOT(toggleChorus (bool)));
      connect(ChorusNumber, SIGNAL (valueChanged (int)), SLOT(changeChorusNumber (int)));
      connect(ChorusType, SIGNAL (activated (int)), SLOT(changeChorusType (int)));
      connect(ChorusSpeed, SIGNAL (valueChanged (int)), SLOT(changeChorusSpeed (int)));
      connect(ChorusDepth, SIGNAL (valueChanged (int)), SLOT(changeChorusDepth (int)));
      connect(ChorusLevel, SIGNAL (valueChanged (int)), SLOT(changeChorusLevel (int)));

      //Clear channels
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++)
            channels[i] = FS_UNSPECIFIED_ID;
      }

FluidSynthGui::~FluidSynthGui()
      {
      /*
      delete _notifier;
      */
      }

void FluidSynthGui::toggleReverb(bool on)         { sendController(0, FS_REVERB_ON, on); }
void FluidSynthGui::changeReverbLevel(int val)    { sendController(0, FS_REVERB_LEVEL, val); }
void FluidSynthGui::changeReverbRoomSize(int val) { sendController(0, FS_REVERB_ROOMSIZE, val); }
void FluidSynthGui::changeReverbWidth(int val)    { sendController(0, FS_REVERB_WIDTH, val); }
void FluidSynthGui::changeReverbDamping(int val)  { sendController(0, FS_REVERB_DAMPING, val); }

void FluidSynthGui::toggleChorus(bool val)        { sendController(0, FS_CHORUS_ON, val); }
void FluidSynthGui::changeChorusNumber(int val)   { sendController(0, FS_CHORUS_NUM, val); }
void FluidSynthGui::changeChorusType(int val)     { sendController(0, FS_CHORUS_TYPE, val); }
void FluidSynthGui::changeChorusSpeed(int val)    { sendController(0, FS_CHORUS_SPEED, val); }
void FluidSynthGui::changeChorusDepth(int val)    { sendController(0, FS_CHORUS_DEPTH, val); }
void FluidSynthGui::changeChorusLevel(int val)    { sendController(0, FS_CHORUS_LEVEL, val); }

      /*

void FluidSynthGui::pushClicked()
      {
      const QString& fns = Filename->text();
      if (fns.isEmpty())
            return;
      const char * fn = fns.toUtf8();

      int datalen = strlen(fn) + 3;
      unsigned char data [datalen];
      data[0] = MUSE_FLUID_SOUNDFONT_PUSH;
      data[1] = MUSE_FLUID_UNSPECIFIED_ID; //This makes the client choose next available external id
      memcpy(data + 2, fn, strlen(fn) + 1 ); //Store filename
      sendSysex(data, datalen);
      data[0] = MUSE_FLUID_GUI_REQ_SOUNDFONTS; //For simplicity's sake, just get all the soundfont data again.
      sendSysex(data, 1);
      printf("Gui sent Sysex.\n");

      return;
      }
      */

void FluidSynthGui::loadClicked()
      {
	QString filename = QFileDialog::getOpenFileName(this, 
                                                      tr("Choose soundfont"), 
                                                      lastdir, 
                                                      //QString("Soundfonts (*.[Ss][Ff]2);;All files (*)"));
                                                      QString("Soundfonts (*.sf2 *.sf3);;All files (*)"));

      if (!filename.isEmpty()) {
            int lastslash = filename.lastIndexOf('/');
            lastdir = filename.left(lastslash);

            sendLastdir(lastdir);
            sendLoadFont(filename);
            }
      }

//---------------------------------------------------------
//   sendLastdir
//   Send the last dir-value to the client
//---------------------------------------------------------

void FluidSynthGui::sendLastdir(QString dir)
      {
      //int l = dir.length()+2;
      int l = dir.length()+4;
      byte data[l];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = FLUIDSYNTH_UNIQUE_ID;
      data[2] = FS_LASTDIR_CHANGE;
      //memcpy(data+1, dir.toUtf8(), dir.length()+1);
      memcpy(data+3, dir.toUtf8().constData(), dir.length()+1);
      sendSysex(data,l);
      }

//---------------------------------------------------------
//   sendUpdateDrumMaps
//   Tell the client to update relevant drum maps
//---------------------------------------------------------

void FluidSynthGui::sendUpdateDrumMaps()
      {
      int l = 3;
      byte data[l];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = MUSE_SYSEX_SYSTEM_ID;
      data[2] = MUSE_SYSEX_SYSTEM_UPDATE_DRUM_MAPS_ID;
      sendSysex(data,l);
      }

//---------------------------------------------------------
//   sendLoadFont
//   Tell the client to load a font with first available id
//---------------------------------------------------------

void FluidSynthGui::sendLoadFont(QString filename)
      {
      //int l = filename.length()+3;
      int l = filename.length()+5;
      byte data[l];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = FLUIDSYNTH_UNIQUE_ID;
      data[2] = FS_PUSH_FONT;
      data[3] = FS_UNSPECIFIED_ID;
      //memcpy(data+2, filename.toUtf8(), filename.length()+1);
      memcpy(data+4, filename.toUtf8().constData(), filename.length()+1);
      sendSysex(data,l);
      }

//---------------------------------------------------------
//   sendLoadFont
//   Tell the client to pop a font
//---------------------------------------------------------

void FluidSynthGui::sendPopFont(int id)
      {
      byte data[4];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = FLUIDSYNTH_UNIQUE_ID;
      data[2] = FS_SOUNDFONT_POP;
      data[3] = id;
      sendSysex(data,4);
      }

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void FluidSynthGui::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      //Sysexes sent from the client
      if (ev.type() == MusECore::ME_SYSEX) {
            const byte* data = ev.constData();
            switch (*data) {
                  case FS_LASTDIR_CHANGE:
                        lastdir = QString((const char*)data+1);
                        break;
                  case FS_ERROR: {
                        char* msg = (char*) (data+1);
                        
                        printf("Muse: fluidsynth error: %s\n", msg);
                        
                        break;
                        }
                  case FS_SEND_SOUNDFONTDATA: {
                        int chunk_len;
                        int filename_len;

                        int count = (int)*(data+1); //Number of elements
                        const byte* cp = data+2; //Point to beginning of first chunk
                        sfListView->clear(); //Clear the listview
                        stack.clear(); //Clear the stack since we're starting over again

                        while (count) {
                              FluidGuiSoundFont font;
                              filename_len = strlen((const char*)cp) + 1;
                              font.name = (const char*)cp;
                              font.id = *(cp + filename_len);
                              chunk_len = filename_len + FS_SFDATALEN;
                              stack.push_front(font);
                              cp += chunk_len; //Move to next chunk
                              count--;
                              }
                        updateSoundfontListView();
                        updateChannelListView();
                        break;
                        }
                  case FS_SEND_CHANNELINFO: {
                        const byte* chptr = (data+1);
                        for (int i=0; i< FS_MAX_NR_OF_CHANNELS; i++) {
                              byte id = *chptr;
                              byte channel = *(chptr+1);
                              channels[channel] = id;
                              chptr+=2;
                              }
                        updateChannelListView();

                        break;
                        }
                  case FS_SEND_DRUMCHANNELINFO: {
                        const byte* drumchptr = (data+1);
                        for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
                              drumchannels[i] = *drumchptr;
                              drumchptr++;
                              }
                        updateChannelListView();
                        break;
                        }
                  default:
                        if (FS_DEBUG)
                              printf("FluidSynthGui::processEvent() : Unknown Sysex received: %d\n", ev.type());
                        break;
                  }
            }
            //Controllers sent from the client:
      else
            if(ev.type() == MusECore::ME_CONTROLLER) {
                  int id = ev.dataA();
                  int val = ev.dataB();
                  switch (id) {
                        case FS_GAIN: {
                              bool sb = Gain->signalsBlocked();
                              Gain->blockSignals(true);
                              // Update Gain-slider without causing it to respond to it's own signal (and send another msg to the synth)
                              Gain->setValue(val);
                              labelGainValue->setNum(val);
                              Gain->blockSignals(sb);
                              break;
                              }
                        case FS_REVERB_ON: {
                              bool sb = Reverb->signalsBlocked();
                              Reverb->blockSignals(true);
                              Reverb->setChecked(val);
                              Reverb->blockSignals(sb);
                              break;
                              }
                        case FS_REVERB_LEVEL: {
                              bool sb = ReverbLevel->signalsBlocked();
                              ReverbLevel->blockSignals(true);
                              ReverbLevel->setValue(val);
                              ReverbLevel->blockSignals(sb);
                              break;
                              }
                        case FS_REVERB_DAMPING: {
                              bool sb = ReverbDamping->signalsBlocked();
                              ReverbDamping->blockSignals(true);
                              ReverbDamping->setValue(val);
                              ReverbDamping->blockSignals(sb);
                              break;
                              }
                        case FS_REVERB_ROOMSIZE: {
                              bool sb = ReverbRoomSize->signalsBlocked();
                              ReverbRoomSize->blockSignals(true);
                              ReverbRoomSize->setValue(val);
                              ReverbRoomSize->blockSignals(sb);
                              break;
                              }
                        case FS_REVERB_WIDTH: {
                              bool sb = ReverbWidth->signalsBlocked();
                              ReverbWidth->blockSignals(true);
                              ReverbWidth->setValue(val);
                              ReverbWidth->blockSignals(sb);
                              break;
                              }
                        case FS_CHORUS_ON: {
                              Chorus->blockSignals(true);
                              Chorus->setChecked(val);
                              Chorus->blockSignals(false);
                              break;
                              }
                        case FS_CHORUS_SPEED: {
                              ChorusSpeed->blockSignals(true);
                              ChorusSpeed->setValue(val);
                              ChorusSpeed->blockSignals(false);
                              break;
                              }
                        case FS_CHORUS_NUM: {
                              ChorusNumber->blockSignals(true);
                              ChorusNumber->setValue(val);
                              ChorusNumber->blockSignals(false);
                              break;
                              }
                        case FS_CHORUS_TYPE: {
                              ChorusType->blockSignals(true);
                              ChorusType->setCurrentIndex(val);
                              ChorusType->blockSignals(false);
                              break;
                              }
                        case FS_CHORUS_DEPTH: {
                              ChorusDepth->blockSignals(true);
                              ChorusDepth->setValue(val);
                              ChorusDepth->blockSignals(false);
                              break;
                              }
                        case FS_CHORUS_LEVEL: {
                              ChorusLevel->blockSignals(true);
                              ChorusLevel->setValue(val);
                              ChorusLevel->blockSignals(false);
                              break;
                              }
                        default:
                              if (FS_DEBUG)
                                    printf("FluidSynthGui::processEvent() : Unknown controller sent to gui: %x\n",id);
                              break;
                        }
                  }
      else
            if (FS_DEBUG)
                  printf("FluidSynthGui::processEvent - unknown event of type %dreceived from synth.\n", ev.type());
      }

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------
void FluidSynthGui::readMessage()
      {
      MessGui::readMessage();
      }

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------
void FluidSynthGui::updateChannelListView()
      {
      if (FS_DEBUG)
            printf("FluidSynthGui::updateChannelListView\n");
      channelListView->clearContents();
      for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
            QString chanstr, sfidstr, drumchanstr;

            //Soundfont id string:
            if (channels[i] == FS_UNSPECIFIED_ID)
                  sfidstr = "unspecified";
            else
                  sfidstr = getSoundFontName(channels[i]);
            //Channel string:
            chanstr = QString::number(i+1);
            if (chanstr.length()==1)
                  chanstr = "0" + chanstr;

            //Drumchan string:
            if (drumchannels[i])
                  drumchanstr = "Yes";
            else
                  drumchanstr = "No";

      const QIcon bd_icon(":/drop_down_triangle.svg");

	    QTableWidgetItem* chan_ = new QTableWidgetItem(chanstr);
	    channelListView->setItem(i, FS_CHANNEL_COL, chan_);
	    QTableWidgetItem* sfid_ = new QTableWidgetItem(bd_icon, sfidstr);
        sfid_->setToolTip("Ctrl to fill all channels\nShift to fill empty channels");
	    channelListView->setItem(i, FS_SF_ID_COL, sfid_);
	    QTableWidgetItem* drum_ = new QTableWidgetItem(bd_icon, drumchanstr);

	    channelListView->setItem(i, FS_DRUM_CHANNEL_COL, drum_);
            }
      channelListView->resizeColumnsToContents();
      }

//---------------------------------------------------------
//   updateSoundfontListView
//---------------------------------------------------------
void FluidSynthGui::updateSoundfontListView()
      {
      sfListView->clear(); //Clear the listview
      for (std::list<FluidGuiSoundFont>::iterator it = stack.begin(); it != stack.end(); it++) {
            QTreeWidgetItem* qlvNewItem = new QTreeWidgetItem(sfListView);
            QString qsid = QString("%1").arg(it->id);
            qlvNewItem->setText(FS_ID_COL, qsid);
            qlvNewItem->setText(FS_SFNAME_COL, QString(it->name));
            sfListView->addTopLevelItem(qlvNewItem);
            }
      sfListView->sortItems(1, Qt::AscendingOrder);
      }

//---------------------------------------------------------
//   changeGain
//---------------------------------------------------------
void FluidSynthGui::changeGain(int value)
      {
      sendController(0, FS_GAIN, value);
      }


//---------------------------------------------------------
//   dumpInfoButton
//---------------------------------------------------------
void FluidSynthGui::dumpInfo()
      {
      //byte data[1];
      byte data[3];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = FLUIDSYNTH_UNIQUE_ID;
      data[2] = FS_DUMP_INFO;
      //sendSysex(data, 1);
      sendSysex(data, 3);
      }

//---------------------------------------------------------
//   getSoundFontName
//---------------------------------------------------------

QString FluidSynthGui::getSoundFontName(int id)
      {
      QString name = nullptr;
      for (std::list<FluidGuiSoundFont>::iterator it = stack.begin(); it != stack.end(); it++) {
            if (id == it->id) {
                  name = it->name;
                  continue;
                  }
            }
      return name;
      }

//---------------------------------------------------------
//   channelItemClicked
// change channel parameters like soundfont / drumchannel on/off
//---------------------------------------------------------

void FluidSynthGui::channelItemClicked(QTableWidgetItem* item)  
      {
      int col = item->column();
      int row = item->row();

      if (col == FS_SF_ID_COL) {
            QMenu* popup = new QMenu(this);
            QPoint ppt = channelListView->visualItemRect(item).bottomLeft();
            QTableWidget* listView = item->tableWidget();
            ppt += QPoint(40, listView->horizontalHeader()->height());
            ppt = listView->mapToGlobal(ppt);

            int i = 0;
            int lastindex = 0;
            for (std::list<FluidGuiSoundFont>::reverse_iterator it = stack.rbegin(); it != stack.rend(); it++) {
                i++;
                /*byte* d = (byte*) it->name.toUtf8();
                  for (int i=0; i<96; i++) {
                        if (i%16 == 0)
                              printf("%x:",(i+d));

                        printf("%x ",*(d-48+i));

                        if (i%16 == 15)
                              printf("\n");
                        }
                  for (int i=0; i<96; i++) {
                        if (i%16 == 0)
                              printf("%x:",(i+d-48));

                        printf("%c ",*(d-48+i));

                        if (i%16 == 15)
                              printf("\n");
                        }
                  printf("\n\n");*/
                QAction* act1 = popup->addAction(it->name);
                act1->setData((int)it->id);
                lastindex = std::max(lastindex, (int)it->id  + 1);
            }

            QAction *lastaction = popup->addAction("unspecified");
            lastaction->setData(lastindex);
            QAction * act = popup->exec(ppt, nullptr);
            if (act) {
                Qt::KeyboardModifiers keymod = qApp->keyboardModifiers();
                int sfid = act->data().toInt();
                QString fontname;
                if (sfid == lastindex) {
                    sfid = FS_UNSPECIFIED_ID;
                    fontname = "unspecified";	//Actually, it's not possible to reset fluid-channels as for now,
                } //so this is just a dummy that makes the synth block any events for the channel
                else {
                    //sfid = getSoundFontId(act->text());
                    fontname = getSoundFontName((byte)sfid);
                }
                if (keymod & (Qt::ShiftModifier|Qt::ControlModifier)) {
                    for (int i = 0; i < FS_MAX_NR_OF_CHANNELS; i++) {
                        if (keymod & Qt::ShiftModifier && channels[i] != FS_UNSPECIFIED_ID && i != row)
                            continue;
                        sendChannelChange((byte)sfid, (byte)i);
                        channels[i] = (byte)sfid;
                        channelListView->item(i, col)->setText(fontname);
                    }
                } else {
                    sendChannelChange((byte)sfid, (byte)row);
                    channels[row] = (byte)sfid;
                    item->setText(fontname);
                }
                sendUpdateDrumMaps();
            }
            delete popup;
      }
         // Drumchannel column:
      else if (col == FS_DRUM_CHANNEL_COL) {
            QMenu* popup = new QMenu(this);
            QPoint ppt = channelListView->visualItemRect(item).bottomLeft();
            QTableWidget* listView = item->tableWidget();
            ppt += QPoint(40, listView->horizontalHeader()->height());
            ppt = listView->mapToGlobal(ppt);
	    QAction * yes = popup->addAction("Yes");
	    yes->setData(1);
            QAction * no = popup->addAction("No");
	    no->setData(0);
            byte channel = row;

            QAction * act2 = popup->exec(ppt, 0);
	    if (act2) {
	          int index = act2->data().toInt();
		  if (index != drumchannels[channel]) {
		        sendDrumChannelChange(index, channel);
                        sendUpdateDrumMaps();
                        drumchannels[channel] = index;
                        item->setText(index == 0 ? "No" : "Yes" );
                        }
	          }
            delete popup;
            }
      }

//---------------------------------------------------------
//   getSoundFontId
//---------------------------------------------------------

int FluidSynthGui::getSoundFontId(QString q)
      {
      int id = -1;
      for (std::list<FluidGuiSoundFont>::iterator it = stack.begin(); it != stack.end(); it++) {
            if (q == it->name)
                  id = it->id;
            }
      return id;
      }

//---------------------------------------------------------
//   sendChannelChange
// Tell the client to set a soundfont to a specific fluid channel
//---------------------------------------------------------

void FluidSynthGui::sendChannelChange(byte font_id, byte channel)
      {
      //byte data[3];
      byte data[5];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = FLUIDSYNTH_UNIQUE_ID;
      data[2] = FS_SOUNDFONT_CHANNEL_SET;
      data[3] = font_id;
      data[4] = channel;
      //sendSysex(data, 3);
      sendSysex(data, 5);
      }

//---------------------------------------------------------
//   sendDrumChannelChange
// Tell the client to set a specific channel to drum channel (equiv to midichan 10)
//---------------------------------------------------------
void FluidSynthGui::sendDrumChannelChange(byte onoff, byte channel)
      {
      //byte data[3];
      byte data[5];
      data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      data[1] = FLUIDSYNTH_UNIQUE_ID;
      data[2] = FS_DRUMCHANNEL_SET;
      data[3] = onoff;
      data[4] = channel;
      //sendSysex(data, 3);
      sendSysex(data, 5);
      if (FS_DEBUG)
            printf("Sent FS_DRUMCHANNEL_SET for channel %d, status: %d\n", channel, onoff);
      }

void FluidSynthGui::popClicked()
      {
      if(currentlySelectedFont >= 0)
      {
        sendPopFont(currentlySelectedFont);
        sendUpdateDrumMaps();
      }
      }

void FluidSynthGui::sfItemClicked(QTreeWidgetItem* item, int /*col*/)
      {
      if (item != 0) {
            currentlySelectedFont = item->text(FS_ID_COL).toInt();
            Pop->setEnabled(true);
            }
      else {
            currentlySelectedFont = -1;
            Pop->setEnabled(false);
            }
      }

#if 0



void FluidSynthGui::readData (int fd)
      {
      unsigned char buffer[512];
      int n = ::read(fd, buffer, 512);
//      dataInput(buffer, n);
      }



void FluidSynthGui::changeReverbRoomSize (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_REVERB,
				"roomsize", value);
}

void FluidSynthGui::changeReverbDamping (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_REVERB,
				"damping", value);
}

void FluidSynthGui::changeReverbWidth (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_REVERB,
				"width", value);
}


void FluidSynthGui::changeChorusNumber (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_CHORUS,
				"number", value);
}

void FluidSynthGui::changeChorusType (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_CHORUS,
				"type", value);
}

void FluidSynthGui::changeChorusSpeed (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_CHORUS,
				"speed", value); //TODO: Right now illegal values may be sent.
				//Make sure they stay within fluidsynths legal boundaries (0.29-5Hz) dunno what that is in doubles
				//This might be the case for the other chorus parameters as well
}

void FluidSynthGui::changeChorusDepth (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_CHORUS,
				"depth", value);
}

void FluidSynthGui::changeChorusLevel (int value) {
	sendParameterChange(MUSE_FLUID_PARAMETER_CHORUS,
				"level", value);
}


void FluidSynthGui::sysexReceived(unsigned char const * data, int len)
      {
      char * cp;
      double * dp;
      //std::cerr << "FluidSynthGui, sysexReceived: " << (int) *data << std::endl;
      switch (*data) {
            case MUSE_FLUID_CLIENT_SEND_PARAMETER:
                  cp = (char *) (data + 2);
                  dp = (double *) (data + strlen (cp) + 3);
                  setParameter ((int) *(data+1), cp, *dp);
                  break;

            case MUSE_FLUID_GAIN_GET:
                  dp = (double *) (data + 1);
                  Gain->setValue ((int) (*dp * 12.8));
                  break;

            case MUSE_FLUID_CLIENT_LASTDIR_CHANGE: {
                  if (*(char*)(data+1) != MUSE_FLUID_UNSPECIFIED_LASTDIR)
                        _lastDir = QString((char*)(data+1));
                  else
                        _lastDir="";
            }

            default:
            break;
      }
      }







void FluidSynthGui::requestAllParameters () {
      unsigned char data[1];

      //data[0] = MUSE_FLUID_ADVGUI_GET;
      //sendSysex (data, 1);
      dbgMsg("Requesting all parameters!\n");
      sendParameterRequest (MUSE_FLUID_PARAMETER_REVERB, "on");
      sendParameterRequest (MUSE_FLUID_PARAMETER_REVERB, "roomsize");
      sendParameterRequest (MUSE_FLUID_PARAMETER_REVERB, "damping");
      sendParameterRequest (MUSE_FLUID_PARAMETER_REVERB, "width");
      sendParameterRequest (MUSE_FLUID_PARAMETER_REVERB, "level");
      sendParameterRequest (MUSE_FLUID_PARAMETER_CHORUS, "on");
      sendParameterRequest (MUSE_FLUID_PARAMETER_CHORUS, "number");
      sendParameterRequest (MUSE_FLUID_PARAMETER_CHORUS, "type");
      sendParameterRequest (MUSE_FLUID_PARAMETER_CHORUS, "speed");
      sendParameterRequest (MUSE_FLUID_PARAMETER_CHORUS, "depth");
      sendParameterRequest (MUSE_FLUID_PARAMETER_CHORUS, "level");
      data[0] = MUSE_FLUID_GAIN_GET;
      sendSysex (data, 1);
      data[0] = MUSE_FLUID_GUI_REQ_SOUNDFONTS;
      sendSysex (data, 1);
}

bool FluidSynthGui::sendParameterRequest (int parameterSet, const char * parameter) {
      size_t parameterMem = strlen (parameter) + 1;
      int datalen = 2 + parameterMem;
      unsigned char * data = new unsigned char [datalen];
      *data = MUSE_FLUID_GUI_REQ_FXPARAMETER_GET;
      *(data + 1) = (char) parameterSet;
      memcpy (data + 2, parameter, parameterMem);
      sendSysex (data, datalen);
      delete data;
      return true;
}

void FluidSynthGui::setParameter (int parameterSet, const char * parameter, double value) {
  int ival = (int) (value * 128);
  std::string ps (parameter);
  if (parameterSet == MUSE_FLUID_PARAMETER_REVERB) {
           if (ps == "roomsize") {
      ReverbRoomSize->setValue (ival);
    } else if (ps == "damping") {
      ReverbDamping->setValue (ival);
    } else if (ps == "width") {
      ReverbWidth->setValue (ival);
    } else if (ps == "level") {
      ReverbLevel->setValue (ival);
    } else if (ps == "on") {
      Reverb->setChecked (ival);
    }
  } else {
           if (ps == "number") {
      ChorusNumber->setValue (ival);
    } else if (ps == "type") {
      ChorusType->setCurrentItem (ival);
    } else if (ps == "speed") {
      ChorusSpeed->setValue (ival);
    } else if (ps == "depth") {
      ChorusDepth->setValue (ival);
    } else if (ps == "level") {
      ChorusLevel->setValue (ival);
    } else if (ps == "on") {
      Chorus->setChecked (ival);
    }
  }
}

//Sends parameter to reverb or chorus
bool FluidSynthGui::sendParameterChange (int parameterSet, const char * parameter, int value) {
  size_t parameterMem = strlen (parameter) + 1;
  int datalen = 2 + parameterMem + sizeof (double);
  unsigned char * data = new unsigned char [datalen];
  *data = (unsigned char) MUSE_FLUID_GUI_REQ_FXPARAMETER_SET;
  *(data + 1) = (unsigned char) parameterSet;
  memcpy (data + 2, parameter, parameterMem);
  double * dp = (double *) (data + 2 + parameterMem);
  *dp = ((double) value) / ((double) 128.0);
  sendSysex (data, datalen);
  delete data;
  return true;
}

void FluidSynthGui::dbgMsg(const char* msg)
      {
      if (MUSE_FLUID_DEBUG)
            std::cerr << msg << std::endl;
      }
//---------------------------------------------------------
//   main
//---------------------------------------------------------

/*QString museProject;
QString museGlobalShare;
QString museUser;*/


int main(int argc, char* argv[])
{
/*
  museUser = getenv("MUSEHOME");
  if (museUser == 0)
    museUser = getenv("HOME");
  museGlobalShare = getenv("MUSE");
  if (museGlobalShare == 0) {
    museGlobalShare = "/usr/muse";
    if (access(museGlobalShare.toUtf8(), R_OK) != 0) {
      museGlobalShare = "/usr/local/muse";
      if (access(museGlobalShare.toUtf8(), R_OK) != 0)
        museGlobalShare = museUser;
    }
  }*/
	char * instanceName = argv[1];
	QApplication app (argc, argv, true);
	QWidget* w = new FluidSynthGui ();
	if (argc > 1)
	w->setCaption(QString(instanceName));
	w->show();
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	qApp->exec();
}

#endif
