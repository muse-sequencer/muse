/*
 * MusE FLUID Synth softsynth plugin
 *
 * Copyright (C) 2004 Mathias Lundgren (lunar_shuttle@users.sourcforge.net)
 *
 * $Id: fluidsynthgui.cpp,v 1.19 2006/01/06 22:48:09 wschweer Exp $
 *
 */

#include "fluidsynthgui.h"
#include "fluidsynti.h"
#include <iostream>
#include "muse/midi.h"
#include "xpm/buttondown.xpm"


//#define MUSE_FLUID_DEBUG      false

FluidSynthGui::FluidSynthGui()
      : MessGui()
      {
      setupUi(this);
      //Connect socketnotifier to fifo
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));
      connect (Push, SIGNAL (clicked()), SLOT(loadClicked()));

      pendingFont = "";

      //channelListView->setColumnWidthMode(FS_CHANNEL_COL, Q3ListView::Maximum);
      //channelListView->setColumnWidthMode(FS_SF_ID_COL,Q3ListView::Maximum);
      ReverbFrame->setEnabled(true);
      ChorusFrame->setEnabled(true);

      if (!FS_DEBUG)
            dumpInfoButton->hide();

      connect(Gain, SIGNAL(valueChanged(int)), SLOT(changeGain(int)));
      connect(dumpInfoButton	, SIGNAL(clicked()), SLOT(dumpInfo()));

      connect(channelTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
         this, SLOT(channelItemClicked(QTreeWidgetItem*,int)));
      connect(sfTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
         this, SLOT(sfItemClicked(QTreeWidgetItem*,int)));

      connect(Reverb, SIGNAL (toggled(bool)), SLOT(toggleReverb(bool)));
      connect(ReverbLevel, SIGNAL (valueChanged (int)), SLOT(changeReverbLevel(int)));
      connect(ReverbRoomSize, SIGNAL (valueChanged (int)), SLOT(changeReverbRoomSize(int)));
      connect(ReverbDamping, SIGNAL (valueChanged (int)), SLOT(changeReverbDamping(int)));
      connect(ReverbWidth, SIGNAL (valueChanged (int)), SLOT(changeReverbWidth(int)));

      connect (Pop, SIGNAL (clicked()), SLOT(popClicked()));
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


void FluidSynthGui::loadClicked()
      {
      QString filename = QFileDialog::getOpenFileName(
         this,
         tr("Choose soundfont"),
         lastdir,
         QString("*.[Ss][Ff]2")
         );
      if (!filename.isEmpty()) {
            QFileInfo fi(filename);
            lastdir = fi.absolutePath();
            sendLastdir(lastdir);
            sendLoadFont(filename);
            pendingFont = filename;
            }
      }

//---------------------------------------------------------
//   sendLastdir
//   Send the last dir-value to the client
//---------------------------------------------------------

void FluidSynthGui::sendLastdir(QString dir)
      {
      int l = dir.size() + 2;
      byte data[l];
      data[0] = FS_LASTDIR_CHANGE;
      memcpy(data+1, dir.toLatin1().data(), dir.size()+1);
      sendSysex(data,l);
      }

//---------------------------------------------------------
//   sendLoadFont
//   Tell the client to load a font with first available id
//---------------------------------------------------------

void FluidSynthGui::sendLoadFont(QString filename)
      {
      int l = filename.length()+3;
      byte data[l];
      data[0] = FS_PUSH_FONT;
      data[1] = FS_UNSPECIFIED_ID;
      memcpy(data+2, filename.toLatin1().data(), filename.length()+1);
      sendSysex(data,l);
      }

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void FluidSynthGui::processEvent(const MidiEvent& ev)
      {
      //Sysexes sent from the client
      if (ev.type() == ME_SYSEX) {
            byte* data = ev.data();
            switch (*data) {
                  case FS_LASTDIR_CHANGE:
                        lastdir = QString((const char*)data+1);
                        break;
                  case FS_ERROR: {
                        char* msg = (char*) (data+1);
                        QMessageBox::critical(this, "Fluidsynth",QString(msg));
                        break;
                        }
                  case FS_SEND_SOUNDFONTDATA: {
                        int chunk_len;
                        int filename_len;

                        int count = (int)*(data+1); //Number of elements
                        byte* cp = data+2; //Point to beginning of first chunk
                        sfTreeWidget->clear(); //Clear the listview
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
                        updateSoundfontTreeWidget();
                        updateChannelTreeWidget();
                        break;
                        }
                  case FS_SEND_CHANNELINFO: {
                        byte* chptr = (data+1);
                        for (int i=0; i< FS_MAX_NR_OF_CHANNELS; i++) {
                              byte id = *chptr;
                              byte channel = *(chptr+1);
                              channels[channel] = id;
                              chptr+=2;
                              }
                        updateChannelTreeWidget();

                        break;
                        }
                  case FS_SEND_DRUMCHANNELINFO: {
                        byte* drumchptr = (data+1);
                        for (int i=0; i<FS_MAX_NR_OF_CHANNELS; i++) {
                              drumchannels[i] = *drumchptr;
                              drumchptr++;
                              }
                        updateChannelTreeWidget();
                        break;
                        }
                  case FS_FONT_SUCCESSFULLY_LOADED: {
                        byte extid = *(data+1);
                        QString fn = (const char*) (data+2);
                        if (FS_DEBUG) {
                              printf("Font successfully loaded: %s, extid: %d\n", fn.toLatin1().data(), extid);
                              }
                        // Try to add last loaded font (if any) to first available channel:
                        if (pendingFont == fn) {
                              if (FS_DEBUG)
                                    printf("Pending font successfully loaded. Add it to first available channel.\n");

                              for (int i=0; i < FS_MAX_NR_OF_CHANNELS; i++) {
                                    if (channels[i] == FS_UNSPECIFIED_ID) {
                                          if (FS_DEBUG)
                                                printf ("sendChannelChange: %d %d\n", extid, i);
                                          sendChannelChange(extid, i);
                                          channels[i] = extid;
                                          updateChannelTreeWidget();
                                          break;
                                          }
                                    }
                              }
                        pendingFont = "";
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
            if(ev.type() == ME_CONTROLLER) {
                  int id = ev.dataA();
                  int val = ev.dataB();
                  switch (id) {
                        case FS_GAIN: {
                              bool sb = Gain->signalsBlocked();
                              Gain->blockSignals(true);
                              // Update Gain-slider without causing it to respond to it's own signal (and send another msg to the synth)
                              Gain->setValue(val);
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
void FluidSynthGui::readMessage(int)
      {
      MessGui::readMessage();
      }

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------
void FluidSynthGui::updateChannelTreeWidget()
      {
      if (FS_DEBUG)
            printf("FluidSynthGui::updateChannelListView\n");
      channelTreeWidget->clear();
      QIcon btndown = QIcon(buttondown_xpm);

      QTreeWidgetItem* header = new QTreeWidgetItem();
      header->setText(FS_CHANNEL_COL, "Channel");
      header->setText(FS_SF_ID_COL, "Soundfont");
      header->setText(FS_DRUM_CHANNEL_COL, "Drum Chnl");
      channelTreeWidget->setHeaderItem(header);

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
            QTreeWidgetItem* qlvNewItem = new QTreeWidgetItem(channelTreeWidget);
            qlvNewItem->setText(FS_CHANNEL_COL, chanstr);
            qlvNewItem->setIcon(FS_SF_ID_COL, btndown);
            qlvNewItem->setText(FS_SF_ID_COL, sfidstr);
            qlvNewItem->setIcon(FS_DRUM_CHANNEL_COL, btndown);
            qlvNewItem->setText(FS_DRUM_CHANNEL_COL, drumchanstr);
            }
      }

//---------------------------------------------------------
//   updateSoundfontTreeWidget
//---------------------------------------------------------
void FluidSynthGui::updateSoundfontTreeWidget()
      {
      sfTreeWidget->clear(); //Clear the listview

      QTreeWidgetItem* header = new QTreeWidgetItem();
      header->setText(FS_ID_COL, "ID");
      header->setText(FS_SFNAME_COL, "Fontname");
      sfTreeWidget->setHeaderItem(header);

      for (std::list<FluidGuiSoundFont>::iterator it = stack.begin(); it != stack.end(); it++) {
            QTreeWidgetItem* qlvNewItem;

            qlvNewItem = new QTreeWidgetItem(sfTreeWidget);
            QString qsid = QString("%1").arg(it->id);
            qlvNewItem->setText(FS_ID_COL, qsid);
            qlvNewItem->setText(FS_SFNAME_COL, QString(it->name));
            }
      //sfTreeWidget->sort();
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
      byte data[1];
      data[0] = FS_DUMP_INFO;
      sendSysex(data, 1);
      }

//---------------------------------------------------------
//   getSoundFontName
//---------------------------------------------------------

QString FluidSynthGui::getSoundFontName(int id)
      {
      QString name = NULL;
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

void FluidSynthGui::channelItemClicked(QTreeWidgetItem* item, int col)
      {
      //
      // Soundfont ID column
      //
      if (col == FS_SF_ID_COL) {
            QMenu* popup = new QMenu(this);
            QPoint ppt = channelTreeWidget->visualItemRect(item).bottomLeft();
            QTreeWidget* treeWidget = item->treeWidget();
            ppt += QPoint(treeWidget->header()->sectionPosition(col), treeWidget->header()->height());
            ppt  = treeWidget->mapToGlobal(ppt);

            int i = 0;
            for (std::list<FluidGuiSoundFont>::reverse_iterator it = stack.rbegin(); it != stack.rend(); it++) {
                  i++;
                  QAction* a = popup->addAction(it->name);
                  a->setData(i);
                  }

            int lastindex = i+1;
            QAction* a = popup->addAction("unspecified");
            a->setData(lastindex);
            a = popup->exec(ppt, 0);
            if (a) {
                  int index = a->data().toInt();
                  byte sfid;
                  QString fontname;
                  if (index == lastindex) {
                        sfid = FS_UNSPECIFIED_ID;
                        fontname = "unspecified";	//Actually, it's not possible to reset fluid-channels as for now,
                        } //so this is just a dummy that makes the synth block any events for the channel
                  else {
                        sfid = getSoundFontId(a->text());
                        fontname = getSoundFontName(sfid);
                        }
                  byte channel = atoi(item->text(FS_CHANNEL_COL).toLatin1().data()) - 1;
                  sendChannelChange(sfid, channel);
                  item->setText(FS_SF_ID_COL, fontname);
                  }
            delete popup;
         }
      //
      // Drumchannel column:
      //
      else if (col == FS_DRUM_CHANNEL_COL) {
            QMenu* popup = new QMenu(this);
            QPoint ppt = channelTreeWidget->visualItemRect(item).bottomLeft();
            QTreeWidget* treeWidget = item->treeWidget();
            ppt += QPoint(treeWidget->header()->sectionPosition(col), treeWidget->header()->height());
            ppt  = treeWidget->mapToGlobal(ppt);
            QAction* a = popup->addAction("Yes");
            a->setData(1);
            a = popup->addAction("No");
            a->setData(0);
            byte channel = atoi(item->text(FS_CHANNEL_COL).toLatin1().data()) - 1;

            a = popup->exec(ppt, 0);
            if (a) {
                  int index = a->data().toInt();
                  if (index != drumchannels[channel]) {
                        sendDrumChannelChange(index, channel);
                        drumchannels[channel] = index;
                        item->setText(FS_DRUM_CHANNEL_COL, index == 0 ? "No" : "Yes" );
                        }
                  }
            }
#if 0
      else if (col == FS_DRUM_CHANNEL_COL) {
            Q3PopupMenu* popup = new Q3PopupMenu(this);
            QPoint ppt = channelListView->itemRect(item).bottomLeft();
            Q3ListView* listView = item->listView();
            ppt += QPoint(listView->header()->sectionPos(col), listView->header()->height());
            ppt  = listView->mapToGlobal(ppt);
            popup->insertItem("Yes", 1);
            popup->insertItem("No", 0);
            byte channel = atoi(item->text(FS_CHANNEL_COL).toLatin1().data()) - 1;

            int index = popup->exec(ppt, 0);
            if (index != drumchannels[channel] && index !=-1) {
                  sendDrumChannelChange(index, channel);
                  drumchannels[channel] = index;
                  item->setText(FS_DRUM_CHANNEL_COL, index == 0 ? "No" : "Yes" );
                  }
            }
#endif
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
      byte data[3];
      data[0] = FS_SOUNDFONT_CHANNEL_SET;
      data[1] = font_id;
      data[2] = channel;
      sendSysex(data, 3);
      }

//---------------------------------------------------------
//   sendDrumChannelChange
// Tell the client to set a specific channel to drum channel (equiv to midichan 10)
//---------------------------------------------------------
void FluidSynthGui::sendDrumChannelChange(byte onoff, byte channel)
      {
      byte data[3];
      data[0] = FS_DRUMCHANNEL_SET;
      data[1] = onoff;
      data[2] = channel;
      sendSysex(data, 3);
      if (FS_DEBUG)
            printf("Sent FS_DRUMCHANNEL_SET for channel %d, status: %d\n", channel, onoff);
      }

void FluidSynthGui::popClicked()
      {
      byte data[2];
      data[0] = FS_SOUNDFONT_POP;
      data[1] = currentlySelectedFont;
      sendSysex(data,2);
      }

void FluidSynthGui::sfItemClicked(QTreeWidgetItem* item, int /*column*/)
      {
      if (item != 0) {
            currentlySelectedFont = atoi(item->text(FS_ID_COL).toLatin1().data());
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

#endif
