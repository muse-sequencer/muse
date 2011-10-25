//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./synti/fluidsynth/fluidsynthgui.h $
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
 * $Id: fluidsynthgui.h,v 1.10.2.3 2009/02/02 21:38:02 terminator356 Exp $
 *
 */

#ifndef __MUSE_FLUIDSYNTHGUI_H__
#define __MUSE_FLUIDSYNTHGUI_H__

#include "ui_fluidsynthguibase.h"
#include "libsynti/gui.h"
#include <list>

class QDialog;
class QTreeWidgetItem;

struct FluidChannel;
#define FS_DEBUG 0 //Turn on/off debug
/*
#include <list>
#include <string>
#include <qscrollview.h>

#include <qevent.h>
#include <qmenubar.h>
#include <qsocketnotifier.h>
#include <alsa/asoundlib.h>
#include <qlistview.h>
#include <qheader.h>
#include "muse/debug.h"
*/

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
//#define FS_INIT_DATA_HEADER_SIZE        4
#define FS_INIT_DATA_HEADER_SIZE        6    // Including MFG + synth IDs
#define FS_INIT_CHANNEL_SECTION       255

// Predefined init-values for fluidsynth
#define FS_PREDEF_VOLUME               0.063
#define FS_PREDEF_REVERB_LEVEL         0.125
#define FS_PREDEF_REVERB_ROOMSIZE      0.125
#define FS_PREDEF_REVERB_DAMPING       0.3
#define FS_PREDEF_REVERB_WIDTH         0.125
#define FS_PREDEF_CHORUS_NUM           3
#define FS_PREDEF_CHORUS_TYPE          1
#define FS_PREDEF_CHORUS_SPEED         0.5
#define FS_PREDEF_CHORUS_DEPTH         0.3
#define FS_PREDEF_CHORUS_LEVEL         0.5
typedef unsigned char byte;


/*


#define MUSE_FLUID_UNSPECIFIED_CHANNEL	127


#define MUSE_FLUID_UNSPECIFIED_LASTDIR	127
*/

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
      FS_DRUMCHANNEL_SET //Used by gui to set drumchannel status for specific channel
      };

enum
      {
      FS_DUMP_INFO = 240,
      FS_ERROR,
      FS_INIT_DATA
      };
/*
enum {
      MUSE_FLUID_REVERB = 100,
      MUSE_FLUID_REVERB_ROOMSIZE,
      MUSE_FLUID_REVERB_DAMPING,
      MUSE_FLUID_REVERB_WIDTH,
      MUSE_FLUID_REVERB_LEVEL,
      MUSE_FLUID_CHORUS,
      MUSE_FLUID_CHORUS_NUMBER,
      MUSE_FLUID_CHORUS_TYPE,
      MUSE_FLUID_CHORUS_SPEED,
      MUSE_FLUID_CHORUS_DEPTH,
      MUSE_FLUID_CHORUS_LEVEL,
      MUSE_FLUID_GAIN,
      MUSE_FLUID_SOUNDFONT,
      MUSE_FLUID_STRING,
      MUSE_FLUID_STRING_END
      };

enum {
      MUSE_FLUID_CLIENT_SEND_PARAMETER = 33,
      MUSE_FLUID_CLIENT_SEND_SOUNDFONTS,
      MUSE_FLUID_PARAMETER_GET,
      MUSE_FLUID_PARAMETER_REVERB,
      MUSE_FLUID_PARAMETER_CHORUS,

      MUSE_FLUID_GAIN_GET,
      MUSE_FLUID_SOUNDFONT_PUSH,
      MUSE_FLUID_SOUNDFONT_POP,

      MUSE_FLUID_CLIENT_SEND_ERROR = 44,
      MUSE_FLUID_SOUNDFONT_LOAD,
      ,
      MUSE_FLUID_CLIENT_RESTORE_CHANNELDATA,
      MUSE_FLUID_CLIENT_INIT_PARAMS,
      MUSE_FLUID_CLIENT_LASTDIR_CHANGE,

      MUSE_FLUID_GUI_REQ_SOUNDFONTS = 60,
      MUSE_FLUID_GUI_REQ_FXPARAMETER_SET,
      MUSE_FLUID_GUI_REQ_FXPARAMETER_GET,
      MUSE_FLUID_GUI_SEND_ERROR,
      MUSE_FLUID_GUI_LASTDIR_CHANGE
      };
*/

struct FluidGuiSoundFont
      {
      QString filename;
      QString name;
      byte id;
      };

//---------------------------------------------------------
//   FluidSynthGui
//---------------------------------------------------------

class FluidSynthGui : public QDialog, public Ui::FLUIDSynthGuiBase, public MessGui
   {
   Q_OBJECT
   private:
      virtual void processEvent(const MusECore::MidiPlayEvent& ev);
      void sendLastdir(QString);
      void sendLoadFont(QString);
      void sendChannelChange(byte font_id, byte channel);
      void sendDrumChannelChange(byte onoff, byte channel);
      void updateSoundfontListView();
      void updateChannelListView();

      QString getSoundFontName(int id);
      int getSoundFontId(QString q);
      QString lastdir;
      std::list<FluidGuiSoundFont> stack;
      byte channels[FS_MAX_NR_OF_CHANNELS]; //Array of bytes, for mapping soundfonts to individual channels
      byte drumchannels[FS_MAX_NR_OF_CHANNELS]; // Array of bytes for setting channels to drumchannels or not (equiv to midichan 10)

      int currentlySelectedFont; //Font currently selected in sfListView. -1 if none selected

/*
      unsigned _smallH;
      unsigned _bigH;
      QSocketNotifier * _notifier;
      bool sendParameterChange (int, const char *, int);
      void setParameter (int, const char *, double);
      void requestAllParameters ();
      void dbgMsg(const char*);
      bool sendParameterRequest(int, const char *);
      //void dealWithSysex (unsigned char const * data, int datalen);






*/
   private slots:
      void loadClicked();
      void readMessage(int);
      void changeGain(int);
      void dumpInfo();
      void channelItemClicked(QTableWidgetItem* item);
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
      void sfItemClicked(QTreeWidgetItem* item, int);
      /*
      void readData(int);




      */

   public:
//      virtual void sysexReceived (const unsigned char *, int);
//    virtual void controllerReceived(int, int, int);

      FluidSynthGui();
      ~FluidSynthGui();
};


#endif /* __MUSE_FLUIDSYNTHGUI_H__ */
