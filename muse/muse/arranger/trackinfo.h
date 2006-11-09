//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __TRACKINFO_H__
#define __TRACKINFO_H__

#include "ui_miditrackinfo.h"
#include "ui_midiportinfo.h"

class Track;
class MidiTrack;
class AudioOutput;
class AudioInput;
class AudioGroup;
class AudioAux;
class SynthI;
class MidiInPort;
class MidiOutPort;
class MidiChannel;
class WaveTrack;
class TLLineEdit;

//---------------------------------------------------------
//   TrackInfo
//---------------------------------------------------------

class TrackInfo : public QWidget {
      Q_OBJECT

      QLabel* label;
      TLLineEdit* name;

   protected:
      QGridLayout* grid;
      Track* track;

   private slots:
      void nameChanged(QString s);
      void songChanged(int);

   public:
      TrackInfo();
      virtual void init(Track*);
      };

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public TrackInfo {
      Q_OBJECT

      Ui::MidiTrackInfoBase mt;
      Ui::MidiPortInfoBase mp;
      QComboBox* port;
      QComboBox* channel;
      QMenu* pop;

   private slots:
      void transpositionChanged(int val);
      void velocityChanged(int val);
      void delayChanged(int val);
      void lenChanged(int val);
      void iKomprChanged(int val);
      void patchClicked();
      void instrumentSelected(int);
      void instrumentChanged();
      void autoChanged(Track*,bool);
      void controllerChanged(int);
      void portSelected(int);
      void deviceIdChanged(int);

   public:
      MidiTrackInfo();
      virtual void init(Track*);
      };
#if 0
//---------------------------------------------------------
//   MidiChannelInfo
//---------------------------------------------------------

class MidiChannelInfo : public TrackInfo {
      Q_OBJECT

      TLLineEdit* portName;
      QComboBox* instrument;
      QPushButton* patch;
      QMenu* pop;

   private slots:
      void instrumentSelected(int);
      void instrumentChanged();
      void controllerChanged(int);
      void patchClicked();

   public:
      MidiChannelInfo();
      virtual void init(Track*);
      };
#endif


//---------------------------------------------------------
//   AudioOutputInfo
//---------------------------------------------------------

class AudioOutputInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioOutputInfo();
      };

//---------------------------------------------------------
//   AudioInputInfo
//---------------------------------------------------------

class AudioInputInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioInputInfo();
      };

//---------------------------------------------------------
//   AudioGroupInfo
//---------------------------------------------------------

class AudioGroupInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioGroupInfo();
      };

//---------------------------------------------------------
//   AudioAuxInfo
//---------------------------------------------------------

class AudioAuxInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioAuxInfo();
      };

//---------------------------------------------------------
//   WaveTrackInfo
//---------------------------------------------------------

class WaveTrackInfo : public TrackInfo {
      Q_OBJECT

   public:
      WaveTrackInfo();
      };

//---------------------------------------------------------
//   SynthIInfo
//---------------------------------------------------------

class SynthIInfo : public TrackInfo {
      Q_OBJECT

   public:
      SynthIInfo();
      };

//---------------------------------------------------------
//   MidiSynthIInfo
//---------------------------------------------------------

class MidiSynthIInfo : public TrackInfo {
      Q_OBJECT

   public:
      MidiSynthIInfo();
      };

//---------------------------------------------------------
//   MidiOutPortInfo
//---------------------------------------------------------

class MidiOutPortInfo : public TrackInfo {
      Q_OBJECT

      Ui::MidiPortInfoBase mp;

   private slots:
      void instrumentSelected(int);
      void instrumentChanged();
      void deviceIdChanged(int);

   public:
      MidiOutPortInfo();
      virtual void init(Track*);
      };

//---------------------------------------------------------
//   MidiInPortInfo
//---------------------------------------------------------

class MidiInPortInfo : public TrackInfo {
      Q_OBJECT

   public:
      MidiInPortInfo();
      };

#endif

