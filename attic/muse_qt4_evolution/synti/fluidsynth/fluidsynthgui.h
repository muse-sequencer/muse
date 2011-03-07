/*
 * MusE FLUID Synth softsynth plugin
 *
 * Copyright (C) 2004 Mathias Lundgren (lunar_shuttle@users.sourcforge.net)
 *
 * $Id: fluidsynthgui.h,v 1.15 2005/10/05 21:51:04 lunar_shuttle Exp $
 *
 */

#ifndef __MUSE_FLUIDSYNTHGUI_H__
#define __MUSE_FLUIDSYNTHGUI_H__

#include "ui_fluidsynthgui.h"
#include "libsynti/gui.h"
#include <list>

struct FluidChannel;
#define FS_DEBUG 0 //Turn on/off debug

#define FS_MAX_NR_OF_CHANNELS          16
#define FS_UNSPECIFIED_FONT           126
#define FS_UNSPECIFIED_ID             127
#define FS_UNSPECIFIED_PRESET         129
#define FS_CHANNEL_COL                  0
#define FS_ID_COL                       0
#define FS_SFNAME_COL                   1
#define FS_SF_ID_COL                    1
#define FS_DRUM_CHANNEL_COL             2

#define FS_SFDATALEN                    1
#define FS_VERSION_MAJOR                0
#define FS_VERSION_MINOR                4
#define FS_INIT_DATA_HEADER_SIZE        4
#define FS_INIT_CHANNEL_SECTION       255

// Predefined init-values for fluidsynth
#define FS_PREDEF_VOLUME               0.2
#define FS_PREDEF_REVERB_LEVEL         0.25
#define FS_PREDEF_REVERB_ROOMSIZE      0.3
#define FS_PREDEF_REVERB_DAMPING       0.3
#define FS_PREDEF_REVERB_WIDTH         0.2
#define FS_PREDEF_CHORUS_NUM           3
#define FS_PREDEF_CHORUS_TYPE          1
#define FS_PREDEF_CHORUS_SPEED         0.5
#define FS_PREDEF_CHORUS_DEPTH         0.3
#define FS_PREDEF_CHORUS_LEVEL         0.5
typedef unsigned char byte;


//Various messages the gui and the client uses to communicate
enum {
      FS_LASTDIR_CHANGE = 1,
      FS_PUSH_FONT
      };

enum {
      //FS_GAIN_SET,
      FS_SEND_SOUNDFONTDATA = 4,
      FS_SEND_CHANNELINFO, //Used by synth to send info about all channels, on init
      FS_SOUNDFONT_CHANNEL_SET,
      FS_SOUNDFONT_POP,
      FS_SEND_DRUMCHANNELINFO, //Used by synth to send drumchannel status about all channels, on init
      FS_DRUMCHANNEL_SET, //Used by gui to set drumchannel status for specific channel
      FS_FONT_SUCCESSFULLY_LOADED // synth tells gui it loaded a font successfully, and gives it it's external id
      };

enum
      {
      FS_DUMP_INFO = 240,
      FS_ERROR,
      FS_INIT_DATA
      };

struct FluidGuiSoundFont
      {
      QString filename;
      QString name;
      byte id;
      };

//---------------------------------------------------------
//   FluidSynthGui
//---------------------------------------------------------

class FluidSynthGui : public QDialog, Ui::FLUIDSynthGuiBase, public MessGui
   {
   Q_OBJECT
   private:
      virtual void processEvent(const MidiEvent& ev);
      void sendLastdir(QString);
      void sendLoadFont(QString);
      void sendChannelChange(byte font_id, byte channel);
      void sendDrumChannelChange(byte onoff, byte channel);
      void updateSoundfontTreeWidget();
      void updateChannelTreeWidget();

      QString getSoundFontName(int id);
      int getSoundFontId(QString q);
      QString lastdir;
      std::list<FluidGuiSoundFont> stack;
      byte channels[FS_MAX_NR_OF_CHANNELS]; //Array of bytes, for mapping soundfonts to individual channels
      byte drumchannels[FS_MAX_NR_OF_CHANNELS]; // Array of bytes for setting channels to drumchannels or not (equiv to midichan 10)

      int currentlySelectedFont; //Font currently selected in sfListView. -1 if none selected
      QString pendingFont;

   private slots:
      void loadClicked();
      void readMessage(int);
      void changeGain(int);
      void dumpInfo();
      void channelItemClicked(QTreeWidgetItem* item, int column);
      void toggleReverb(bool);
      void changeReverbLevel (int);
      void changeReverbRoomSize(int val);
      void changeReverbWidth(int val);
      void changeReverbDamping(int val);
      void toggleChorus(bool);
      void changeChorusNumber(int);
      void changeChorusType(int);
      void changeChorusSpeed(int);
      void changeChorusDepth(int);
      void changeChorusLevel(int);
      void popClicked();
      void sfItemClicked(QTreeWidgetItem* item, int column);

   public:
//      virtual void sysexReceived (const unsigned char *, int);
//    virtual void controllerReceived(int, int, int);

      FluidSynthGui();
      ~FluidSynthGui();
};


#endif /* __MUSE_FLUIDSYNTHGUI_H__ */
